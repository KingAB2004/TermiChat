#include"P2P_TermiChat.h"
void ConnectionRequest(){
    int h=7, w=50, y=(LINES-h)/2, x=(COLS-w)/2;
                    WINDOW* win = newwin(h,w,y,x); box(win,0,0);
                    mvwprintw(win,1,2,"%s wants to connect.", peer_username.c_str());
                    mvwprintw(win,3,4,"[Y]es   [N]o");
                    wrefresh(win);

                    int ch; bool accepted=false;
                    keypad(win, TRUE);
                    nodelay(win, FALSE);
                    while (true) {
                        ch = wgetch(win);
                        if (ch=='y' || ch=='Y') { accepted=true; break; }
                        if (ch=='n' || ch=='N' ) { break; }
                    }
                    werase(win);     // clear window content
                    wrefresh(win);   // update the screen
                    delwin(win);
                    
                    int client_sock =SocketStore.front();
                    SocketStore.pop();
                    if (accepted) {
                        vector<unsigned char> me(my_username.begin(), my_username.end());
                        send_packet(client_sock, PT_CONNECT_ACCEPT, me);
                        {
                            unique_lock<mutex>lock(Queue_mutex);
                            commandQueue.push("ConnectionAccept");
                        }
                    } else {
                        send_packet(client_sock, PT_CONNECT_REJECT, {});
                    }

}