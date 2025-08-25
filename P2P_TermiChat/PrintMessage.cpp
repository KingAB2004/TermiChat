#include"P2P_TermiChat.h"

//  for Printing the Messages
void print_message(const string &msg) {
    std::unique_lock<std::mutex> lock(Queue_mutex);
    if (!chat_win) return;
    lock.unlock();
    commandQueue.push("TextMessage");
    TextStore.push((msg + "\n").c_str());
    lock.lock();

}