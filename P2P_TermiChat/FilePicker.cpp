#include"P2P_TermiChat.h"



//  ///Selecting the Files using this function from the computer///
// Return the file path to the file or the empty string if no file is selected or the folder is restricted to enter
string file_picker(string start_dir) {
    vector<string> entries;
    string cwd = start_dir;

    while (true) {
        entries.clear();
        DIR* d = opendir(cwd.c_str());
        if (!d) return "";
        struct dirent* e;
        while ((e = readdir(d)) != nullptr) {
            string name = e->d_name;
            if (name == ".") continue;
            entries.push_back(name);
        }
        closedir(d);
        sort(entries.begin(), entries.end());

        int h = max(10, LINES-6), w = max(30, COLS-10), y = (LINES-h)/2, x = (COLS-w)/2;
        WINDOW* win = newwin(h, w, y, x); box(win,0,0);
        mvwprintw(win,1,2,"Select file in: %s", cwd.c_str());
        int highlight=0; keypad(win, TRUE); nodelay(win, FALSE);

        while (true) {
            // clear list area
            for (int i=3;i<h-2;i++) {
                for (int j=1;j<w-1;j++) mvwaddch(win,i,j,' ');
            }
            // draw
            int maxRows = h-5;
            for (int i=0; i<(int)entries.size() && i<maxRows; ++i) {
                if (i==highlight) wattron(win, A_REVERSE);
                mvwprintw(win, i+3, 2, "%s", entries[i].c_str());
                if (i==highlight) wattroff(win, A_REVERSE);
            }
            mvwprintw(win, h-2, 2, "Enter=open/select  Backspace=..  Esc=cancel");
            
            wrefresh(win);
            int ch = wgetch(win);
            if (ch == KEY_UP)    highlight = (highlight - 1 + (int)entries.size()) % (int)entries.size();
            
            else if (ch == KEY_DOWN) highlight = (highlight + 1) % (int)entries.size();
            else if (ch == 10) { // Enter
                string pick = cwd + "/" + entries[highlight];
                
                struct stat st{};
                if (stat(pick.c_str(), &st)==0) {
                    if (S_ISDIR(st.st_mode)) {
                        delwin(win);
                        cwd = pick;
                        break; // refresh listings
                    } else if (S_ISREG(st.st_mode)) {
                        delwin(win);
                        return pick; // file chosen
                    }
                }
                delwin(win);
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                size_t slash = cwd.find_last_of('/');
                if (slash != string::npos && slash > 0) cwd = cwd.substr(0, slash);
                else cwd = "/";
                delwin(win);
                break;
            } else if (ch == 27) { // Esc
                delwin(win);
                return "";
            }
        }
    }
}