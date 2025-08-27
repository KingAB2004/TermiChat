#include"P2P_TermiChat.h"

void ConnectionRequest(){
    int h=7, w=50, y=(LINES-h)/2, x=(COLS-w)/2;
    // Opens a new window for collecting the input of y or n about accepting the connection 
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
    // Then we get the Sock we stored in the SocketStore in the lsitner 
    int client_sock =SocketStore.front();
    SocketStore.pop();
    if (accepted) {
        vector<unsigned char> me(my_username.begin(), my_username.end());
        //  Send the packet of acceptace and push the connection Accpet in the Queue
        if(!send_packet(client_sock, PT_CONNECT_ACCEPT, me))endwin();
        {
            unique_lock<mutex>lock(Queue_mutex);
            commandQueue.push("ConnectionAccept");
        }
        // Store the Currect Active Sock in this
        active_sock =client_sock;
    } else {
        send_packet(client_sock, PT_CONNECT_REJECT, {});
    }

}