#include<ncurses.h>
#include<string>
#include<vector>
#include"./HandleSQL/ListFriends.h"
#include"./HandleSQL/AddFriends.h"
#include"./HandleUserName/UserName.h"
#include"./P2P_TermiChat/P2P_TermiChat.h"
using namespace std;
WINDOW* chat_win = nullptr;
WINDOW* input_win = nullptr;
mutex chat_mutex;
mutex Queue_mutex;
queue<string>commandQueue;
queue<string>TextStore;
bool running = true;
string peer_ip;
sqlite3* db = nullptr;
AES_Encryptor* aes = nullptr;
vector<unsigned char> key(32, 0x01);
vector<unsigned char> iv(16, 0x02);
string my_username;
string peer_username;
int LISTEN_PORT = 50000;
Friend f;

 queue<int>SocketStore;


int main(){

    // Getting the UserName from the Database
     my_username =getOrCreateUsername();

    // Initializing the Screen
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    curs_set(0);
        int height = LINES-3, width = COLS;

        chat_win = newwin(height,width,0,0);
    
    input_win = newwin(3,width,height,0);
    scrollok(chat_win,TRUE);
    string home = getenv("HOME")? getenv("HOME") : ".";
    string db_path = home + "/Public/TermiChat/friend.db";

    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return 0;
    }
    // Setting the Key and initialization vector
    aes = new AES_Encryptor(key,iv);
    
    box(input_win,0,0);
    thread(listener_thread, LISTEN_PORT).detach();

    std::vector<std::string> menu = {"Add Friend","List Friends","Start a Chat","Start Group Chat","Exit"};

    int choice;
    int highlight = 0;
    string peer_ip;


// The Program will Run until the user dont select to exit 

    while (true) {
        {
            unique_lock<mutex> lock(Queue_mutex);
            while(!commandQueue.empty())
            {
                string s =commandQueue.front();
                commandQueue.pop();
                lock.unlock();
                if(s == "AddFriend")
                {
                    AddFriend();
                }
                else if(s == "getListOfFriends")
                {
                    getListOfFriends();
                }
                else if(s == "StartChat")
                {
                    StartChat(my_username);
                }
                else if(s == "ConnectionRequest")
                {
                    ConnectionRequest();
                }
                else if(s == "ConnectionAccept")
                {
                    clear();
                    sender_thread(peer_ip);
                }
                else if(s== "TextMessage")
                {
                    string k =TextStore.front();
                    TextStore.pop();
                waddstr(chat_win, k.c_str());
                wrefresh(chat_win);
                }
                else if(s== "GotAccepted")
                {
                    clear();
                     // Build chat windows once accepted
                    int height = LINES-3, width = COLS;

                    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
                    wrefresh(chat_win); wrefresh(input_win);

                    // Show previous history
                    display_previous_messages(f.name);

                    // sending until not exited
                    sender_thread(f.ip);

                    // cleaning up
                    endwin();
                }
                lock.lock();
            }
        }
        clear();
        mvprintw(0, 0, "=== TermiChat ===");
        for (int i = 0; i < menu.size(); i++) {
            if (i == highlight) {
                attron(A_REVERSE);
            }
            mvprintw(i + 2, 2, menu[i].c_str());
            if (i == highlight) {
                attroff(A_REVERSE);
            }
        }

        choice = getch();
        switch (choice) {
            case KEY_UP:
                highlight = (highlight - 1 + menu.size()) % menu.size();
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % menu.size();
                break;
            case 10: 
            {
                unique_lock<mutex> lock(Queue_mutex);
                lock.unlock();
                if (highlight == 0) {
                    commandQueue.push("AddFriend");
                } else if (highlight == 1) {
                    commandQueue.push("getListOfFriends");
                } else if (highlight == 2) {
                    commandQueue.push("StartChat");
                } 
                else if (highlight == 3) {
                    // Will Be pushed in later Versions
                    mvprintw(menu.size() + 3, 0, "You chose Start Group Chat But Sorry it will be pushed in later versions");
                }
                else if (highlight == 4) {
                    endwin();
                    return 0;
                }
                break;
                lock.lock();
            }
        }
    }

    endwin();
    return 0;
}


