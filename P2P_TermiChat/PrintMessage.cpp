#include"P2P_TermiChat.h"

//  for Printing the Messages
void print_message(const string &msg){
    lock_guard<mutex> lock(chat_mutex);
    if (chat_win) {
        wprintw(chat_win, "%s\n", msg.c_str());
        wrefresh(chat_win);
    }
}