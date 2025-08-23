#include"AddFriends.h"
#include<sqlite3.h>
#include<filesystem>
#include<iostream>
#include<ncurses.h>
using namespace std;

// Function for Adding the Friend into the DataBase

void AddFriend() {
    clear();
    echo();
    mvprintw(0, 2, "=== Add Friend ===");

    // Getting the Userinfo

    char name[100] ,ip[100];
    mvprintw(0 ,0 ,"Write the name of the Friend\n");
    getstr(name);
    clear();
    mvprintw(0 ,0 ,"Write the IP of the Friend\n");
    getstr(ip);
    clear();
    noecho();

    // Getting the Database 

    string s(name) ,IP(ip);
    string home = getenv("HOME");
    string db_path = home + "/Public/TermiChat/friend.db";
    sqlite3* db;
    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database\n";
        return;
    }

    // Inserting into the DataBase using the exec Command 

    int start_row = 2;
    char* err_msg = nullptr;
    string text ="Insert into friends (name ,ip) values('"+s+"','"+IP+"');";
    if (sqlite3_exec(db, text.c_str(), nullptr, &start_row, &err_msg) != SQLITE_OK) {
        mvprintw(start_row + 1, 2, "SQL Error: %s", err_msg);
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);
    mvprintw(0, 2, "Congratulations for Making a new Friend!!!\n");
    mvprintw(start_row + 3, 2, "Press any key to return to menu...");
    getch();
}

