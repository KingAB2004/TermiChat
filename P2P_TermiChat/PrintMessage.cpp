#include"P2P_TermiChat.h"

//  for Printing the Messages
void print_message(const std::string &msg) {
    if (!chat_win) return;
    std::lock_guard<std::mutex> lock(chat_mutex);  // protect ncurses access
    waddstr(chat_win, (msg + "\n").c_str());
    wrefresh(chat_win);
}
