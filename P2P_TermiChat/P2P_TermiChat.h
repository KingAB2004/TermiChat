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
#include"../Encryption/Encryptor.h"
#define BUF_SIZE 1024
#include"../Encryption/Encryptor.h"

using namespace std;
struct Friend {
    string name;
    string ip;
};
extern WINDOW *chat_win;
extern WINDOW *input_win;
extern mutex chat_mutex;
extern bool running;

extern sqlite3* db;
extern AES_Encryptor* aes;

extern vector<unsigned char> key;
extern vector<unsigned char> iv;

extern string my_username;
extern string peer_username;

extern int LISTEN_PORT;

extern Friend f;

// This is the Packet which the user or the friend sends to confirm whether he wants to connect or not 
// Also it Tells wheather the sent Message is a file or text
 enum PacketType : unsigned char {
    PT_TEXT            = 0,
    PT_FILE            = 1,
    PT_CONNECT_REQUEST = 2,
    PT_CONNECT_ACCEPT  = 3,
    PT_CONNECT_REJECT  = 4
};


void StartChat(string username);
string file_picker(string dir);
void save_message(const string &friend_name, const string &sender,const string &type, const vector<unsigned char> &data);
void print_message(const string &msg);
void display_previous_messages(const string &friend_name);
 bool send_packet(int sock, PacketType t, const std::vector<unsigned char>& DAta);
 bool receivingPacket(int sock, PacketType& t, std::vector<unsigned char>& DAta) ;
void listener_thread(int port);
void sender_thread(const string &friend_ip);
#endif
