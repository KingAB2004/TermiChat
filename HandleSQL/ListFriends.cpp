#include"ListFriends.h"
#include<sqlite3.h>
#include<filesystem>
#include<iostream>
#include<ncurses.h>

using namespace std;


// CallBack function which gives parsed row by row data from the table of friends 

static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    int *row = static_cast<int*>(NotUsed);
        int id = argv[0] ? atoi(argv[0]) : 0;
    const char* name = argv[1] ? argv[1] : "NULL";
    const char* ip = argv[2] ? argv[2] : "NULL";

    mvprintw(*row, 2, "id: %-3d name: %-20s ip: %-15s", id, name, ip);  // Here 3 20 15 are the alignment for the text
    (*row)++;
    return 0;
}

// Main function to get the List of the friends from the data base

void getListOfFriends() {
    clear();
    mvprintw(0, 2, "=== Friends List ===");

    string home = getenv("HOME");
    string db_path = home + "/Public/TermiChat/friend.db";
    sqlite3* db;
    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database\n";
        return;
    }

    int start_row = 2;
    char* err_msg = nullptr;
    if (sqlite3_exec(db, "SELECT * FROM friends;", callback, &start_row, &err_msg) != SQLITE_OK) {
        mvprintw(start_row + 1, 2, "SQL Error: %s", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
    mvprintw(start_row + 3, 2, "Press any key to return to menu...");
    getch();
}

