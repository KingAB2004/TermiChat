
#include"P2P_TermiChat.h"

//  NetWorking Part Here We have the Listner as well as the Sender Thread as it is a P2P System so both as server as well as a Client 
void listener_thread(int port){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(port);

    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,5);
    while(running){
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(server_fd,(struct sockaddr*)&client_addr,&addrlen);
        if(client_sock<0) continue;
        char ip_str[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
std::string peer_ip(ip_str);

        thread([client_sock ,peer_ip](){
            PacketType t; vector<unsigned char> DAta;

            while (running) {
                if (!receivingPacket(client_sock, t, DAta)) break;


                // Here when the Connection Request is received it opens a window in the terminal That Provides us with 2 options and if we select yes then it sends the Connection Accept packet to the sender
                if (t == PT_CONNECT_REQUEST) {
                    string requester(DAta.begin(), DAta.end());

                    int h=7, w=50, y=(LINES-h)/2, x=(COLS-w)/2;
                    WINDOW* win = newwin(h,w,y,x); box(win,0,0);
                    mvwprintw(win,1,2,"%s wants to connect.", requester.c_str());
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
                    delwin(win);

                    if (accepted) {
                        peer_username = requester;
                        f.name =requester;
                        sender_thread(peer_ip);
                        vector<unsigned char> me(my_username.begin(), my_username.end());
                            initscr(); cbreak(); noecho(); keypad(stdscr, TRUE);
                            mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
    wrefresh(chat_win); wrefresh(input_win);
                        send_packet(client_sock, PT_CONNECT_ACCEPT, me);
                        break;
                    } else {
                        send_packet(client_sock, PT_CONNECT_REJECT, {});
                        break;
                    }
                }
                // If he sends a text Message then the data is first decrypted and Shows the message in the Chatwindow
                else if (t == PT_TEXT) {
                    vector<unsigned char> enc = DAta;
        
                    vector<unsigned char> dec = aes->decrypt(enc);
                    string msg(dec.begin(), dec.end());
        
                    string label = peer_username.empty()? "Friend" : peer_username;
                    print_message(label + ": " + msg);
                    save_message(label ,label,"text",enc);
                }
                // If he sends a file then file is first decrypted and then stored in the ChatFiles folder in the TermiChat Folder 
                else if (t == PT_FILE) {
                    vector<unsigned char> enc = DAta;
                    vector<unsigned char> dec = aes->decrypt(enc);

                    string outdir = string(getenv("HOME")? getenv("HOME") : ".") + "/Public/TermiChat/ChatFiles";
        
                    mkdir(outdir.c_str(), 0755);  // 0755 represents the permissons the owner have on this file  ,group have and others have
                    string outpath = outdir + "/recv_" + to_string((long long)time(nullptr));
        
                    ofstream outfile(outpath, ios::binary); // Creating a Output File in the outpath
                    string label = peer_username.empty()? "Friend" : peer_username;
                    if (outfile) {
                        outfile.write((char*)dec.data(), (std::streamsize)dec.size()); //  Saving the data in the Outfile
                        outfile.close();
        
                        print_message(label + " sent a file → " + outpath);
                    } else {
                        print_message("Failed to save received file.");
                    }
                    save_message(label,label,"file",enc);
                }
                else {
                    if(t == PT_CONNECT_ACCEPT)
                    {
                        string fri(DAta.begin() ,DAta.end());
                        f.name =fri;
                        
                    }
                }
            }
            close(client_sock);
        }).detach();
    }
    close(server_fd);
}
