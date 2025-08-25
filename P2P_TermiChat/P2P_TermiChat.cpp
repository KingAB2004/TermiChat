#include "P2P_TermiChat.h"
#include "../Encryption/Encryptor.h"

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
    return true;
}



// Now this is the main function that handles all the operations of P2P Chat for our Users
void StartChat(string username){
    my_username = username;

    // Init ncurses early for friend selection UI
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE);

    

    f = select_friend(db);
    if (f.name.empty()) {
        endwin(); sqlite3_close(db); db=nullptr; delete aes; aes=nullptr; return;
    }
    peer_username=f.name;
    peer_ip =f.ip;
    clear(); refresh();
    mvprintw(0, 0, "Preparing to connect to %s (%s)...", f.name.c_str(), f.ip.c_str());
    refresh();

    // Start listener first

    mvprintw(2, 0, "Sending connect request..."); refresh();

    request_connection_and_wait(f.ip, LISTEN_PORT);
        mvprintw(4, 0, "Connection Request Sent so if the Friend Accepts the Chat will be Connected");
        refresh();
        getch();
}
