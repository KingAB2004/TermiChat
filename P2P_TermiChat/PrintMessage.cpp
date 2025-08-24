#include"P2P_TermiChat.h"

//  for Printing the Messages
void print_message(const string &msg) {
    std::lock_guard<std::mutex> lk(chat_mutex);
    if (!chat_win) return;
    waddstr(chat_win, (msg + "\n").c_str());
    wrefresh(chat_win);
}