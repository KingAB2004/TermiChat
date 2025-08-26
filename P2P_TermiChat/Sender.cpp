#include"P2P_TermiChat.h"

// This is the Sender Thread that handles the Sending of messages along with managing the ui of ncurses

void sender_thread(const string &friend_ip){
    while(running){
        werase(input_win);
        box(input_win,0,0);
        
        mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
        
        wrefresh(input_win);

        // allows F2 usage
        keypad(input_win, TRUE);
        nodelay(input_win, FALSE);

        // Line editor that supports F2 for file picker
        string msg;
        {
            char buffer[BUF_SIZE] = {0};
            int idx=0;
            while (true) {
                int ch = wgetch(input_win);
                if (ch == KEY_F(2)) {
                    string start = string(getenv("HOME")? getenv("HOME") : ".");
                    string path = file_picker(start);
                    if (!path.empty()) {
                        int sock = socket(AF_INET,SOCK_STREAM,0);
        
                        sockaddr_in serv_addr{};
                        serv_addr.sin_family=AF_INET;
        
                        serv_addr.sin_port=htons(LISTEN_PORT);
                        inet_pton(AF_INET,peer_ip.c_str(),&serv_addr.sin_addr);
        
                        if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){ // Connecting the socket
                            print_message("Failed to connect to " + friend_ip);
                            close(sock);
                        } else {
                            vector<unsigned char> filedata = AES_Encryptor::read_file(path);
        
                            vector<unsigned char> enc = aes->encrypt(filedata);
                            send_packet(sock, PT_FILE, enc);
        
                            print_message("You sent file: " + path);
                            save_message(f.name, "You", "file", enc);
                            close(sock);
                        }
                    }
                    // get back to the input field
                    werase(input_win); box(input_win,0,0);
        
                    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
                    wrefresh(input_win);
        
                    idx = 0; buffer[0]=0; // reset input
                    continue;
                } else if (ch == '\n') {
                    buffer[idx] = 0;
                    msg = buffer;
                    break;
                } else if (ch == KEY_BACKSPACE || ch == 127) {
                    if (idx>0) { idx--; buffer[idx]=0; }
                } else if (isprint(ch) && idx < BUF_SIZE-1) {
                    buffer[idx++] = (char)ch; buffer[idx]=0;
                }
                // re-render text after label
                for (int i=20;i<COLS-2;i++) mvwaddch(input_win,1,i,' ');
                mvwprintw(input_win,1,20,"%s", buffer);
                wrefresh(input_win);
            }
        }

        if(msg=="/quit"){ running=false; break; }

        // Allow /file <path> as a command too
        if(msg.rfind("/file ",0)==0 && msg.size()>6){
            string filename = msg.substr(6);
            vector<unsigned char> filedata = AES_Encryptor::read_file(filename);
        
            vector<unsigned char> enc = aes->encrypt(filedata);

            int sock = socket(AF_INET,SOCK_STREAM,0);
            sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(LISTEN_PORT);
            inet_pton(AF_INET,peer_ip.c_str(),&serv_addr.sin_addr);
        


            if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
                print_message("Failed to connect to " + friend_ip);
                close(sock);
            } else {
        
                send_packet(sock, PT_FILE, enc);
        
                print_message("You sent file: "+filename);
        
                save_message(f.name,"You","file",enc);
                close(sock);
            }
            continue;
        }

        int sock = socket(AF_INET,SOCK_STREAM,0);
        
        sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(LISTEN_PORT);
        
        inet_pton(AF_INET,peer_ip.c_str(),&serv_addr.sin_addr);
        if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
            print_message("Failed to connect to " + friend_ip);
            close(sock);
            continue;
        }
        vector<unsigned char> text(msg.begin(),msg.end());
        
        vector<unsigned char> enc = aes->encrypt(text);
        send_packet(sock, PT_TEXT, enc);
        
        print_message(string("You: ") + msg);
        save_message(f.name,"me","text",enc);
        close(sock);
    }
}
