#include"P2P_TermiChat.h"

//  for Printing the Messages
void print_message(const string &msg) {
    std::lock_guard<std::mutex> lock(chat_mutex);
    if (!chat_win) return;
    waddstr(chat_win,msg.c_str()+'\n');
    wrefresh(chat_win);

}