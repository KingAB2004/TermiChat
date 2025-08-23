#include<ncurses.h>
#include<string>
#include<vector>
#include"./HandleSQL/ListFriends.h"
#include"./HandleSQL/AddFriends.h"
#include"./HandleUserName/UserName.h"
#include"./P2P_TermiChat/P2P_TermiChat.h"


using namespace std;

int main(){

    // Getting the UserName from the Database
    string UserName =getOrCreateUsername();

    // Initializing the Screen
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);
    curs_set(0);

    std::vector<std::string> menu = {"Add Friend","List Friends","Start a Chat","Start Group Chat","Exit"};

    int choice;
    int highlight = 0;


// The Program will Run until the user dont select to exit 

    while (true) {
        clear();
        mvprintw(0, 0, "=== TermiChat ===");
        for (int i = 0; i < menu.size(); i++) {
            if (i == highlight) {
                attron(A_REVERSE);
            }
            mvprintw(i + 2, 2, menu[i].c_str());
            if (i == highlight) {
                attroff(A_REVERSE);
            }
        }

        choice = getch();
        switch (choice) {
            case KEY_UP:
                highlight = (highlight - 1 + menu.size()) % menu.size();
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % menu.size();
                break;
            case 10: 
                if (highlight == 0) {
                    AddFriend();
                } else if (highlight == 1) {
                    getListOfFriends();
                } else if (highlight == 2) {
                    StartChat(UserName);
                } 
                else if (highlight == 3) {
                    // Will Be pushed in later Versions
                    mvprintw(menu.size() + 3, 0, "You chose Start Group Chat");
                }
                 else if (highlight == 4) {
                    endwin();
                    return 0;
                }
                getch();
                break;
        }
    }

    endwin();
    return 0;
}


