#include "P2P_TermiChat.h"
#include "../Encryption/Encryptor.h"
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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <cctype>

#define BUF_SIZE 1024

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

// ////Helper Functions//// 

// This is the Packet which the user or the friend sends to confirm whether he wants to connect or not 
// Also it Tells wheather the sent Message is a file or text
enum PacketType : unsigned char {
    PT_TEXT            = 0,
    PT_FILE            = 1,
    PT_CONNECT_REQUEST = 2,
    PT_CONNECT_ACCEPT  = 3,
    PT_CONNECT_REJECT  = 4
};

static void safe_print_stdout(const string& s) {
    fprintf(stdout, "%s\n", s.c_str());
    fflush(stdout);
}

//  for Printing the Messages
void print_message(const string &msg){
    lock_guard<mutex> lock(chat_mutex);
    if (chat_win) {
        wprintw(chat_win, "%s\n", msg.c_str());
        wrefresh(chat_win);
    } else {
        safe_print_stdout(msg);
    }
}
// It for sending a packet using the socket given along with the data you have to send 

// Scoket Structure  ->  Packet Type 1byte  | Payload Length() 4 Byte | Payload Data Payload.size()
static bool send_packet(int sock, PacketType t, const std::vector<unsigned char>& DAta) {
    // here the htonl is used to convert it into network Style
    uint32_t nsize = htonl((uint32_t)DAta.size());
    
    if (write(sock, &t, 1) != 1) return false;
    
    if (write(sock, &nsize, 4) != 4) return false;
    size_t off = 0, left = DAta.size();

    while (left) {

        ssize_t n = write(sock, DAta.data() + off, left);
        if (n <= 0) return false;
        off += (size_t)n; left -= (size_t)n;
    }
    return true;
}

static bool recv_fully(int sock, void* buf, size_t len) {
    char* p = (char*)buf;
//    Checking if received fully or not
    size_t got = 0;
    while (got < len) {
   
        ssize_t n = read(sock, p + got, len - got);
   
        if (n <= 0) return false;
        got += (size_t)n;
    }
    return true;
}

// Function for Receiving the Packet as ordered by the socket structure

static bool receivingPacket(int sock, PacketType& t, std::vector<unsigned char>& DAta) {
    unsigned char tbyte;
    uint32_t nsize_net;
    if (!recv_fully(sock, &tbyte, 1)) return false;
   
    if (!recv_fully(sock, &nsize_net, 4)) return false;
   
    uint32_t size = ntohl(nsize_net);
    DAta.resize(size);
    if (size) {
   
        if (!recv_fully(sock, DAta.data(), size)) return false;
    }
    t = (PacketType)tbyte;
    return true;
}


// For Saving the Messages into the DataBase

void save_message(const string &friend_name, const string &sender,
                  const string &type, const vector<unsigned char> &data){
    sqlite3_stmt* stmt = nullptr;
    string query = "INSERT INTO messages(friend_name,sender,type,content) VALUES(?,?,?,?);";
   
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
   
        print_message(string("DB prepare failed: ") + sqlite3_errmsg(db));
        return;
    }
    // Replacing the ? in the Statement with the Info of the Message
    sqlite3_bind_text(stmt,1,friend_name.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_text(stmt,2,sender.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_text(stmt,3,type.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_blob(stmt,4,data.data(),(int)data.size(),SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        print_message(string("DB step failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

// This function is used to Display Previous Message History

void display_previous_messages(const string &friend_name){
    sqlite3_stmt* stmt = nullptr;
    
    string query = "SELECT sender,type,content FROM messages WHERE friend_name=? ORDER BY timestamp;";
    
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        print_message(string("DB prepare failed: ") + sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt,1,friend_name.c_str(),-1,SQLITE_TRANSIENT);

    while (sqlite3_step(stmt)==SQLITE_ROW){
        string sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
        
        string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        
        const void* blob = sqlite3_column_blob(stmt,2);
        
        int size = sqlite3_column_bytes(stmt,2);
        vector<unsigned char> data((unsigned char*)blob,(unsigned char*)blob+size);
// If the data is a text then show the Message
        if(type=="text"){
            vector<unsigned char> dec = aes->decrypt(data);
            string text(dec.begin(),dec.end());
            print_message(sender + ": " + text);
        } else if(type=="file"){
            // IF the Type is a file just show that he sent a file
            print_message(sender + " sent a file.");
        }
    }
    sqlite3_finalize(stmt);
}

//  //// Selecting a Friend ////

Friend select_friend(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    vector<Friend> friends;
    // To Connect to the Friend we need both the name and the ip
    if (sqlite3_prepare_v2(db, "SELECT name, ip FROM friends;", -1, &stmt, NULL) != SQLITE_OK) {
        print_message(string("Failed to prepare statement: ") + sqlite3_errmsg(db));
        return {"", ""};
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Friend f;
        f.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        f.ip   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        friends.push_back(f);
    }
    sqlite3_finalize(stmt);

    if (friends.empty()) {
        print_message("No friends found in database!");
        return {"", ""};
    }
    // Selecting the Friend Based on key Movements and using Enter to finalize it
    int highlight = 0, ch;
    while (true) {
        clear();
        mvprintw(0, 0, "Select a friend (Enter to confirm):");
        for (int i = 0; i < (int)friends.size(); i++) {
            if (i == highlight) attron(A_REVERSE);
            mvprintw(i + 2, 2, "%s (%s)", friends[i].name.c_str(), friends[i].ip.c_str());
            if (i == highlight) attroff(A_REVERSE);
        }
        ch = getch();
        if (ch == KEY_UP) highlight = (highlight - 1 + (int)friends.size()) % (int)friends.size();
        else if (ch == KEY_DOWN) highlight = (highlight + 1) % (int)friends.size();
        else if (ch == 10) return friends[highlight]; 
    }
}

//  ///Selecting the Files using this function from the computer///
// Return the file path to the file or the empty string if no file is selected or the folder is restricted to enter
string file_picker(string start_dir) {
    vector<string> entries;
    string cwd = start_dir;

    while (true) {
        entries.clear();
        DIR* d = opendir(cwd.c_str());
        if (!d) return "";
        struct dirent* e;
        while ((e = readdir(d)) != nullptr) {
            string name = e->d_name;
            if (name == ".") continue;
            entries.push_back(name);
        }
        closedir(d);
        sort(entries.begin(), entries.end());

        int h = max(10, LINES-6), w = max(30, COLS-10), y = (LINES-h)/2, x = (COLS-w)/2;
        WINDOW* win = newwin(h, w, y, x); box(win,0,0);
        mvwprintw(win,1,2,"Select file in: %s", cwd.c_str());
        int highlight=0; keypad(win, TRUE); nodelay(win, FALSE);

        while (true) {
            // clear list area
            for (int i=3;i<h-2;i++) {
                for (int j=1;j<w-1;j++) mvwaddch(win,i,j,' ');
            }
            // draw
            int maxRows = h-5;
            for (int i=0; i<(int)entries.size() && i<maxRows; ++i) {
                if (i==highlight) wattron(win, A_REVERSE);
                mvwprintw(win, i+3, 2, "%s", entries[i].c_str());
                if (i==highlight) wattroff(win, A_REVERSE);
            }
            mvwprintw(win, h-2, 2, "Enter=open/select  Backspace=..  Esc=cancel");
            
            wrefresh(win);
            int ch = wgetch(win);
            if (ch == KEY_UP)    highlight = (highlight - 1 + (int)entries.size()) % (int)entries.size();
            
            else if (ch == KEY_DOWN) highlight = (highlight + 1) % (int)entries.size();
            else if (ch == 10) { // Enter
                string pick = cwd + "/" + entries[highlight];
                
                struct stat st{};
                if (stat(pick.c_str(), &st)==0) {
                    if (S_ISDIR(st.st_mode)) {
                        delwin(win);
                        cwd = pick;
                        break; // refresh listings
                    } else if (S_ISREG(st.st_mode)) {
                        delwin(win);
                        return pick; // file chosen
                    }
                }
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                size_t slash = cwd.find_last_of('/');
                if (slash != string::npos && slash > 0) cwd = cwd.substr(0, slash);
                else cwd = "/";
                delwin(win);
                break;
            } else if (ch == 27) { // Esc
                delwin(win);
                return "";
            }
        }
    }
}

//  /// Sending a Connection Request to the Friend And then If the Friend Accepts the Connection the ChatWindow Opens 
bool request_connection_and_wait(const string& friend_ip, int friend_port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock < 0) return false;
    sockaddr_in serv{}; serv.sin_family = AF_INET; serv.sin_port = htons(friend_port);
    // Converting the String Address to Binary which can be correctly interpreted as the network Byte order 
    inet_pton(AF_INET, friend_ip.c_str(), &serv.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        close(sock); return false;
    }
    vector<unsigned char> name_bytes(my_username.begin(), my_username.end());
    
    if (!send_packet(sock, PT_CONNECT_REQUEST, name_bytes)) { close(sock); return false; }

    PacketType t; vector<unsigned char> DAta;
    
    if (!receivingPacket(sock, t, DAta)) { close(sock); return false; }

    if (t == PT_CONNECT_ACCEPT) {
        peer_username.assign(DAta.begin(), DAta.end());
        session_active = true;
        close(sock);
        return true;
    } else {
        close(sock);
        return false;
    }
}

//  NetWorking Part Here We have the Listner as well as the Sender Thread as it is a P2P System so both as server as well as a Client 

void listener_thread(int port ,const string &friend_name){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(port);

    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,5);
    print_message("Listening on port " + to_string(port));

    while(running){
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(server_fd,(struct sockaddr*)&client_addr,&addrlen);
        if(client_sock<0) continue;

        thread([client_sock, friend_name](){
            PacketType t; vector<unsigned char> DAta;

            while (running) {
                if (!receivingPacket(client_sock, t, DAta)) break;


                // Here when the Connection Request is received it opens a window in the terminal That Provides us with 2 options and if we select yes then it sends the Connection Accept packet to the sender
                if (t == PT_CONNECT_REQUEST) {
                    string requester(DAta.begin(), DAta.end());

                    int h=7, w=50, y=(LINES-h)/2, x=(COLS-w)/2;
                    WINDOW* win = newwin(h,w,y,x); box(win,0,0);
                    mvwprintw(win,1,2,"%s wants to connect.", requester.c_str());
                    mvwprintw(win,3,4,"[Y]es   [N]o");
                    wrefresh(win);

                    int ch; bool accepted=false;
                    keypad(win, TRUE);
                    nodelay(win, FALSE);
                    while (true) {
                        ch = wgetch(win);
                        if (ch=='y' || ch=='Y') { accepted=true; break; }
                        if (ch=='n' || ch=='N' || ch==27) { break; }
                    }
                    delwin(win);

                    if (accepted) {
                        peer_username = requester;
                        session_active = true;
                        vector<unsigned char> me(my_username.begin(), my_username.end());
                        send_packet(client_sock, PT_CONNECT_ACCEPT, me);
                        break;
                    } else {
                        send_packet(client_sock, PT_CONNECT_REJECT, {});
                        break;
                    }
                }
                // If he sends a text Message then the data is first decrypted and Shows the message in the Chatwindow
                else if (t == PT_TEXT) {
                    vector<unsigned char> enc = DAta;
        
                    vector<unsigned char> dec = aes->decrypt(enc);
                    string msg(dec.begin(), dec.end());
        
                    string label = peer_username.empty()? "Friend" : peer_username;
                    print_message(label + ": " + msg);
                    save_message(friend_name ,friend_name,"text",enc);
                }
                // If he sends a file then file is first decrypted and then stored in the ChatFiles folder in the TermiChat Folder 
                else if (t == PT_FILE) {
                    vector<unsigned char> enc = DAta;
                    vector<unsigned char> dec = aes->decrypt(enc);

                    string outdir = string(getenv("HOME")? getenv("HOME") : ".") + "/Public/TermiChat/ChatFiles";
        
                    mkdir(outdir.c_str(), 0755);  // 0755 represents the permissons the owner have on this file  ,group have and others have
                    string outpath = outdir + "/recv_" + to_string((long long)time(nullptr));
        
                    ofstream outfile(outpath, ios::binary); // Creating a Output File in the outpath
                    if (outfile) {
                        outfile.write((char*)dec.data(), (std::streamsize)dec.size()); //  Saving the data in the Outfile
                        outfile.close();
        
                        string label = peer_username.empty()? "Friend" : peer_username;
                        print_message(label + " sent a file → " + outpath);
                    } else {
                        print_message("Failed to save received file.");
                    }
                    save_message(friend_name,friend_name,"file",enc);
                }
                else {
                    // PT_CONNECT_ACCEPT / PT_CONNECT_REJECT: is ignored here
                }
            }
            close(client_sock);
        }).detach();
    }
    close(server_fd);
}

// This is the Sender Thread that handles the Sending of messages along with managing the ui of ncurses

void sender_thread(const string &friend_ip,int friend_port ,const string &friend_name){
    while(running){
        werase(input_win);
        box(input_win,0,0);
        
        mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
        
        wrefresh(input_win);

        // allows F2 usage
        keypad(input_win, TRUE);
        nodelay(input_win, FALSE);

        // Line editor that supports F2 for file picker
        string msg;
        {
            char buffer[BUF_SIZE] = {0};
            int idx=0;
            while (true) {
                int ch = wgetch(input_win);
                if (ch == KEY_F(2)) {
                    string start = string(getenv("HOME")? getenv("HOME") : ".");
                    string path = file_picker(start);
                    if (!path.empty()) {
                        int sock = socket(AF_INET,SOCK_STREAM,0);
        
                        sockaddr_in serv_addr{};
                        serv_addr.sin_family=AF_INET;
        
                        serv_addr.sin_port=htons(friend_port);
                        inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        
                        if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){ // Connecting the socket
                            print_message("Failed to connect to " + friend_ip);
                            close(sock);
                        } else {
                            vector<unsigned char> filedata = AES_Encryptor::read_file(path);
        
                            vector<unsigned char> enc = aes->encrypt(filedata);
                            send_packet(sock, PT_FILE, enc);
        
                            print_message("You sent file: " + path);
                            save_message(friend_name, "me", "file", enc);
                            close(sock);
                        }
                    }
                    // get back to the input field
                    werase(input_win); box(input_win,0,0);
        
                    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
                    wrefresh(input_win);
        
                    idx = 0; buffer[0]=0; // reset input
                    continue;
                } else if (ch == '\n') {
                    buffer[idx] = 0;
                    msg = buffer;
                    break;
                } else if (ch == KEY_BACKSPACE || ch == 127) {
                    if (idx>0) { idx--; buffer[idx]=0; }
                } else if (isprint(ch) && idx < BUF_SIZE-1) {
                    buffer[idx++] = (char)ch; buffer[idx]=0;
                }
                // re-render text after label
                for (int i=20;i<COLS-2;i++) mvwaddch(input_win,1,i,' ');
                mvwprintw(input_win,1,20,"%s", buffer);
                wrefresh(input_win);
            }
        }

        if(msg=="/quit"){ running=false; break; }

        // Allow /file <path> as a command too
        if(msg.rfind("/file ",0)==0 && msg.size()>6){
            string filename = msg.substr(6);
            vector<unsigned char> filedata = AES_Encryptor::read_file(filename);
        
            vector<unsigned char> enc = aes->encrypt(filedata);

            int sock = socket(AF_INET,SOCK_STREAM,0);
            sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(friend_port);
            inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        
            if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
                print_message("Failed to connect to " + friend_ip);
                close(sock);
            } else {
        
                send_packet(sock, PT_FILE, enc);
        
                print_message("You sent file: "+filename);
        
                save_message(friend_name,"me","file",enc);
                close(sock);
            }
            continue;
        }

        int sock = socket(AF_INET,SOCK_STREAM,0);
        
        sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(friend_port);
        
        inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
            print_message("Failed to connect to " + friend_ip);
            close(sock);
            continue;
        }
        vector<unsigned char> text(msg.begin(),msg.end());
        
        vector<unsigned char> enc = aes->encrypt(text);
        send_packet(sock, PT_TEXT, enc);
        
        print_message(string("You: ") + msg);
        save_message(friend_name,"me","text",enc);
        close(sock);
    }
}



// Now this is the main function that handles all the operations of P2P Chat for our Users
void StartChat(string username){
    my_username = username;

    // Init ncurses early for friend selection UI
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE);

    string home = getenv("HOME")? getenv("HOME") : ".";
    string db_path = home + "/Public/TermiChat/friend.db";

    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return;
    }
    // Setting the Key and initialization vector
    aes = new AES_Encryptor(key,iv);

    Friend f = select_friend(db);
    if (f.name.empty()) {
        endwin(); sqlite3_close(db); db=nullptr; delete aes; aes=nullptr; return;
    }

    clear(); refresh();
    mvprintw(0, 0, "Preparing to connect to %s (%s)...", f.name.c_str(), f.ip.c_str());
    refresh();

    // Start listener first
    thread(listener_thread, LISTEN_PORT ,f.name).detach();

    mvprintw(2, 0, "Sending connect request..."); refresh();

    if (!request_connection_and_wait(f.ip, LISTEN_PORT)) {
        mvprintw(4, 0, "Connection rejected or failed. Press any key.");
        refresh();
        getch();
        endwin(); sqlite3_close(db); db=nullptr; delete aes; aes=nullptr; return;
    }

    // Build chat windows once accepted
    int height = LINES-3, width = COLS;
    chat_win = newwin(height,width,0,0);
    
    input_win = newwin(3,width,height,0);
    scrollok(chat_win,TRUE);
    
    box(input_win,0,0);
    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
    wrefresh(chat_win); wrefresh(input_win);

    // Show previous history
    display_previous_messages(f.name);

    // sending until not exited
    sender_thread(f.ip, LISTEN_PORT ,f.name);

    // cleaning up
    endwin();
    sqlite3_close(db); db=nullptr;
    delete aes; aes=nullptr;
    chat_win = nullptr; input_win = nullptr;
}
