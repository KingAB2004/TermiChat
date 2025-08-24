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
bool running = true;
atomic<bool> in_chat{false};
sqlite3* db = nullptr;
AES_Encryptor* aes = nullptr;
vector<unsigned char> key(32, 0x01);
vector<unsigned char> iv(16, 0x02);
string my_username;
string peer_username;
int LISTEN_PORT = 50000;
std::mutex connq_m;
std::queue<ConnReq> connq;
std::condition_variable connq_cv;
Friend f={"Alice" ,"10.50.40.250"};
std::atomic<bool> chat_active{false}; 

int main(){

    // Getting the UserName from the Database
    string UserName =getOrCreateUsername();

    // Initializing the Screen
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    curs_set(0);
        string home = getenv("HOME")? getenv("HOME") : ".";
        string db_path = home + "/Public/TermiChat/friend.db";

    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return 0;
    }
        int height = LINES-3, width = COLS;

        chat_win = newwin(height,width,0,0);
        aes = new AES_Encryptor(key,iv);

    input_win = newwin(3,width,height,0);
    scrollok(chat_win,TRUE);
    
    box(input_win,0,0);
    thread(listener_thread, LISTEN_PORT).detach();

    std::vector<std::string> menu = {"Add Friend","List Friends","Start a Chat","Start Group Chat","Exit"};

    int choice;
    int highlight = 0;


// The Program will Run until the user dont select to exit 

while (true) {
    // 1) Handle any pending incoming connection requests
    ConnReq req;
    bool have_req = false;
    {
        std::lock_guard<std::mutex> lk(connq_m);
        if (!connq.empty()) {
            req = connq.front();
            connq.pop();
            have_req = true;
        }
    }

    if (have_req) {
        // Show Y/N accept dialog on main thread (safe)
        int h = 7, w = 50, y = (LINES - h) / 2, x = (COLS - w) / 2;
        WINDOW *win = newwin(h, w, y, x);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "%s wants to connect.", req.requester.c_str());
        mvwprintw(win, 3, 4, "[Y]es   [N]o");
        wrefresh(win);

        int ch;
        bool accepted = false;
        keypad(win, TRUE);
        nodelay(win, FALSE);
        while (true) {
            ch = wgetch(win);
            if (ch == 'y' || ch == 'Y') { accepted = true; break; }
            if (ch == 'n' || ch == 'N') { break; }
        }
        delwin(win);

        if (accepted) {
            // Update peer state
            peer_username = req.requester;
            f.name = req.requester;
            // send accept over the accepted socket
            std::vector<unsigned char> me(my_username.begin(), my_username.end());
            if (!send_packet(req.client_sock, PT_CONNECT_ACCEPT, me)) {
                print_message("Failed to send CONNECT_ACCEPT to " + req.peer_ip);
                close(req.client_sock);
                // continue menu loop
                continue;
            }
            // Close the accepted socket: requester already handles it (request_connection_and_wait closes after reading accept)
            close(req.client_sock);

            // Prepare and enter chat UI on main thread
            in_chat.store(true);
            chat_active.store(true);

            // set-up chat windows (reuse StartChat's UI setup or inline a minimal setup)
            clear(); refresh();
            int height = LINES - 3, width = COLS;
            if (!chat_win) chat_win = newwin(height, width, 0, 0);
            if (!input_win) input_win = newwin(3, width, height, 0);
            scrollok(chat_win, TRUE);
            box(input_win, 0, 0);
            mvwprintw(input_win, 1, 2, "[F2: Send File]  Type here:");
            wrefresh(chat_win);
            wrefresh(input_win);

            // display history if you have it
            display_previous_messages(f.name);

            // Run sender loop in the main thread — blocks here until chat ends
            sender_thread(req.peer_ip);

            // sender_thread returned -> chat ended
            chat_active.store(false);
            in_chat.store(false);

            // clear chat windows visually for menu redraw
            std::lock_guard<std::mutex> lk(chat_mutex);
            werase(chat_win); wrefresh(chat_win);
            werase(input_win); wrefresh(input_win);

            // continue to menu redraw
            continue;
        } else {
            // reject and close socket
            send_packet(req.client_sock, PT_CONNECT_REJECT, {});
            close(req.client_sock);
            continue;
        }
    }

    // 2) if no pending request, draw menu and handle input as usual
    if (in_chat.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue; // let chat own UI/input
    }

    // draw menu (header + username + items)
    clear();
    mvprintw(0, 0, "=== TermiChat ===");
    mvprintw(1, 0, "User: %s", UserName.c_str());
    for (int i = 0; i < (int)menu.size(); ++i) {
        int row = i + 3;
        if (i == highlight) attron(A_REVERSE);
        mvprintw(row, 2, "%s", menu[i].c_str());
        if (i == highlight) attroff(A_REVERSE);
    }
    refresh();

    int choice = getch();
    switch (choice) {
        case KEY_UP: highlight = (highlight - 1 + menu.size()) % menu.size(); break;
        case KEY_DOWN: highlight = (highlight + 1) % menu.size(); break;
        case 10:
            if (highlight == 0) { AddFriend(); getch(); }
            else if (highlight == 1) { getListOfFriends(); getch(); }
            else if (highlight == 2) {
                in_chat.store(true);
                StartChat(UserName);   // StartChat will perform an outgoing handshake and then call sender_thread
                in_chat.store(false);
            }
            else if (highlight == 3) { mvprintw(menu.size() + 5, 0, "Start Group Chat (not implemented)"); getch(); }
            else if (highlight == 4) {
                // cleanup and exit
                if (db) { sqlite3_close(db); db = nullptr; }
                if (aes) { delete aes; aes = nullptr; }
                endwin();
                return 0;
            }
            break;
    }
}
    endwin();
if (db) { sqlite3_close(db); db = nullptr; }
if (aes) { delete aes; aes = nullptr; }
    return 0;
}


