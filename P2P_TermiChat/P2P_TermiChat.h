#ifndef P2P_H
#define P2P_H

#include <sqlite3.h>
#include <ncurses.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include<string>
#include<vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <cctype>
#define BUF_SIZE 1024
#include"../Encryption/Encryptor.h"

using namespace std;
struct Friend {
    string name;
    string ip;
};

//  Defining the Global variables
WINDOW *chat_win = nullptr, *input_win = nullptr;
mutex chat_mutex;
bool running = true;

sqlite3* db = nullptr;
AES_Encryptor* aes = nullptr;
// Keys can be changed based on your choice
vector<unsigned char> key(32, 0x01);
vector<unsigned char> iv(16, 0x02);

string my_username;
string peer_username;
atomic<bool> session_active{false};

// Listening port can be choosen based on which port is free on both the computers
int LISTEN_PORT = 50000;


// This is the Packet which the user or the friend sends to confirm whether he wants to connect or not 
// Also it Tells wheather the sent Message is a file or text
enum PacketType : unsigned char {
    PT_TEXT            = 0,
    PT_FILE            = 1,
    PT_CONNECT_REQUEST = 2,
    PT_CONNECT_ACCEPT  = 3,
    PT_CONNECT_REJECT  = 4
};

Friend f;

void StartChat(string username);
string file_picker(string dir);
void save_message(const string &friend_name, const string &sender,const string &type, const vector<unsigned char> &data);
void print_message(const string &msg);
void display_previous_messages(const string &friend_name);
static bool send_packet(int sock, PacketType t, const std::vector<unsigned char>& DAta);
static bool receivingPacket(int sock, PacketType& t, std::vector<unsigned char>& DAta) ;
void listener_thread(int port);
#endif
