#include "UserName.h"
#include <ncurses.h>
#include <iostream>
using namespace std;

// Checking if the UserName is Stored in the database or not

bool hasUsername(sqlite3* db, string& username) {
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        if (argc > 0 && argv[0]) {
            string* uname = static_cast<string*>(data);
            *uname = argv[0]; 
        }
        return 0; 
    };

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, "SELECT username FROM users LIMIT 1;", callback, &username, &errmsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errmsg << endl;
        sqlite3_free(errmsg);
        return false;
    }

    return !username.empty(); 
}

// If UserName is not Stored in the Database then Push it into the Database

void insertUsername(sqlite3* db, const string& username) {
    string query = "INSERT INTO users(username) VALUES('" + username + "');";
    sqlite3_exec(db, query.c_str(), 0, 0, 0);
}

// Main Function That will be Imported Later and Gives the UserName

string getOrCreateUsername() {
    string home = getenv("HOME");
    string db_path = home + "/Public/TermiChat/friend.db";
    sqlite3* db;
    if (sqlite3_open(db_path.c_str(), &db)) {
        endwin();
        cerr << "Cannot open database\n";
        return "";
    }
    string username;
    initscr();
    cbreak();
    echo();
    keypad(stdscr, TRUE);
    curs_set(0);

    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users(username TEXT);", 0, 0, 0);

    if (!hasUsername(db, username)) {
        mvprintw(0, 0, "=== TermiChat ===");
        mvprintw(1, 0, "Enter your username: ");
        char buf[100];
        getnstr(buf, 99);
        username = string(buf);
        insertUsername(db, username);
        mvprintw(2, 0, "Username saved! Press any key to continue...");
        getch();
    } else {
        mvprintw(0, 0, ("Welcome To TermiChat, " + username).c_str());
        getch();
    }

    endwin();
    return username;
}
