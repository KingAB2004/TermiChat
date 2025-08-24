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

    PacketType t; vector<unsigned char> DAta;
    
    if (!receivingPacket(sock, t, DAta)) { close(sock); return false; }

    if (t == PT_CONNECT_ACCEPT) {
        peer_username.assign(DAta.begin(), DAta.end());
        close(sock);
        return true;
    } else {
        close(sock);
        return false;
    }
}



// Now this is the main function that handles all the operations of P2P Chat for our Users
void StartChat(string username){
    my_username = username;
    print_message("DEBUG: Entered StartChat for " + username);

    // take UI ownership
    clear(); refresh();

    int height = LINES - 3, width = COLS;
    if (!chat_win) chat_win = newwin(height, width, 0, 0);
    if (!input_win) input_win = newwin(3, width, height, 0);
    scrollok(chat_win, TRUE);

    box(input_win, 0, 0);
    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
    wrefresh(chat_win); wrefresh(input_win);

    display_previous_messages(f.name);

    // Mark chat active so menu pauses
    chat_active.store(true);

    // Ensure a listener thread is running (you already start it in main)
    // Run sender loop in THIS thread so StartChat blocks while chat is active
    sender_thread(f.ip);

    // When sender_thread returns, chat_active should be false
    chat_active.store(false);
    print_message("DEBUG: Leaving StartChat for " + username);

    // cleanup windows visually but DO NOT endwin() or close DB
    std::lock_guard<std::mutex> lg(chat_mutex);
    werase(chat_win); wrefresh(chat_win);
    werase(input_win); wrefresh(input_win);

}
