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
        session_active = true;
        close(sock);
        return true;
    } else {
        close(sock);
        return false;
    }
}

//  NetWorking Part Here We have the Listner as well as the Sender Thread as it is a P2P System so both as server as well as a Client 

void listener_thread(int port ,const string &friend_name){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(port);

    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,5);
    print_message("Listening on port " + to_string(port));

    while(running){
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(server_fd,(struct sockaddr*)&client_addr,&addrlen);
        if(client_sock<0) continue;

        thread([client_sock, friend_name](){
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
                        if (ch=='n' || ch=='N' || ch==27) { break; }
                    }
                    delwin(win);

                    if (accepted) {
                        peer_username = requester;
                        session_active = true;
                        vector<unsigned char> me(my_username.begin(), my_username.end());
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
                    save_message(friend_name ,friend_name,"text",enc);
                }
                // If he sends a file then file is first decrypted and then stored in the ChatFiles folder in the TermiChat Folder 
                else if (t == PT_FILE) {
                    vector<unsigned char> enc = DAta;
                    vector<unsigned char> dec = aes->decrypt(enc);

                    string outdir = string(getenv("HOME")? getenv("HOME") : ".") + "/Public/TermiChat/ChatFiles";
        
                    mkdir(outdir.c_str(), 0755);  // 0755 represents the permissons the owner have on this file  ,group have and others have
                    string outpath = outdir + "/recv_" + to_string((long long)time(nullptr));
        
                    ofstream outfile(outpath, ios::binary); // Creating a Output File in the outpath
                    if (outfile) {
                        outfile.write((char*)dec.data(), (std::streamsize)dec.size()); //  Saving the data in the Outfile
                        outfile.close();
        
                        string label = peer_username.empty()? "Friend" : peer_username;
                        print_message(label + " sent a file → " + outpath);
                    } else {
                        print_message("Failed to save received file.");
                    }
                    save_message(friend_name,friend_name,"file",enc);
                }
                else {
                    // PT_CONNECT_ACCEPT / PT_CONNECT_REJECT: is ignored here
                }
            }
            close(client_sock);
        }).detach();
    }
    close(server_fd);
}

// This is the Sender Thread that handles the Sending of messages along with managing the ui of ncurses

void sender_thread(const string &friend_ip,int friend_port ,const string &friend_name){
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
        
                        serv_addr.sin_port=htons(friend_port);
                        inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        
                        if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){ // Connecting the socket
                            print_message("Failed to connect to " + friend_ip);
                            close(sock);
                        } else {
                            vector<unsigned char> filedata = AES_Encryptor::read_file(path);
        
                            vector<unsigned char> enc = aes->encrypt(filedata);
                            send_packet(sock, PT_FILE, enc);
        
                            print_message("You sent file: " + path);
                            save_message(friend_name, "me", "file", enc);
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
            sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(friend_port);
            inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        
            if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
                print_message("Failed to connect to " + friend_ip);
                close(sock);
            } else {
        
                send_packet(sock, PT_FILE, enc);
        
                print_message("You sent file: "+filename);
        
                save_message(friend_name,"me","file",enc);
                close(sock);
            }
            continue;
        }

        int sock = socket(AF_INET,SOCK_STREAM,0);
        
        sockaddr_in serv_addr{}; serv_addr.sin_family=AF_INET; serv_addr.sin_port=htons(friend_port);
        
        inet_pton(AF_INET,friend_ip.c_str(),&serv_addr.sin_addr);
        if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
            print_message("Failed to connect to " + friend_ip);
            close(sock);
            continue;
        }
        vector<unsigned char> text(msg.begin(),msg.end());
        
        vector<unsigned char> enc = aes->encrypt(text);
        send_packet(sock, PT_TEXT, enc);
        
        print_message(string("You: ") + msg);
        save_message(friend_name,"me","text",enc);
        close(sock);
    }
}



// Now this is the main function that handles all the operations of P2P Chat for our Users
void StartChat(string username){
    my_username = username;

    // Init ncurses early for friend selection UI
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE);

    string home = getenv("HOME")? getenv("HOME") : ".";
    string db_path = home + "/Public/TermiChat/friend.db";

    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        return;
    }
    // Setting the Key and initialization vector
    aes = new AES_Encryptor(key,iv);

    Friend f = select_friend(db);
    if (f.name.empty()) {
        endwin(); sqlite3_close(db); db=nullptr; delete aes; aes=nullptr; return;
    }
    thread(listener_thread, LISTEN_PORT ,f.name).detach();

    clear(); refresh();
    mvprintw(0, 0, "Preparing to connect to %s (%s)...", f.name.c_str(), f.ip.c_str());
    refresh();

    // Start listener first

    mvprintw(2, 0, "Sending connect request..."); refresh();

    if (!request_connection_and_wait(f.ip, LISTEN_PORT)) {
        mvprintw(4, 0, "Connection rejected or failed. Press any key.");
        refresh();
        getch();
        endwin(); sqlite3_close(db); db=nullptr; delete aes; aes=nullptr; return;
    }

    // Build chat windows once accepted
    int height = LINES-3, width = COLS;
    chat_win = newwin(height,width,0,0);
    
    input_win = newwin(3,width,height,0);
    scrollok(chat_win,TRUE);
    
    box(input_win,0,0);
    mvwprintw(input_win,1,2,"[F2: Send File]  Type here:");
    wrefresh(chat_win); wrefresh(input_win);

    // Show previous history
    display_previous_messages(f.name);

    // sending until not exited
    sender_thread(f.ip, LISTEN_PORT ,f.name);

    // cleaning up
    endwin();
    sqlite3_close(db); db=nullptr;
    delete aes; aes=nullptr;
    chat_win = nullptr; input_win = nullptr;
}
