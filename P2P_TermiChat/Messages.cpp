#include"P2P_TermiChat.h"

// For Saving the Messages into the DataBase

void save_message(const string &friend_name, const string &sender,
                  const string &type, const vector<unsigned char> &data){
    sqlite3_stmt* stmt = nullptr;
    string query = "INSERT INTO messages(friend_name,sender,type,content) VALUES(?,?,?,?);";
   
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        print_message(string("DB prepare failed: ") + sqlite3_errmsg(db));
        return;
    }
    // Replacing the ? in the Statement with the Info of the Message
    sqlite3_bind_text(stmt,1,friend_name.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_text(stmt,2,sender.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_text(stmt,3,type.c_str(),-1,SQLITE_TRANSIENT);
   
    sqlite3_bind_blob(stmt,4,data.data(),(int)data.size(),SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        print_message(string("DB step failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

// This function is used to Display Previous Message History

void display_previous_messages(const string &friend_name){
    sqlite3_stmt* stmt = nullptr;
    
    string query = "SELECT sender,type,content FROM messages WHERE friend_name=? ORDER BY timestamp;";
    
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        // print_message("Hello");
        print_message(string("DB prepare failed: ") + sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt,1,friend_name.c_str(),-1,SQLITE_TRANSIENT);

    while (sqlite3_step(stmt)==SQLITE_ROW){
        string sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
        
        string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        
        const void* blob = sqlite3_column_blob(stmt,2);
        
        int size = sqlite3_column_bytes(stmt,2);
        vector<unsigned char> data((unsigned char*)blob,(unsigned char*)blob+size);
// If the data is a text then show the Message
        if(type=="text"){
            vector<unsigned char> dec = aes->decrypt(data);
            string text(dec.begin(),dec.end());
            print_message(sender + ": " + text);
        } else if(type=="file"){
            // IF the Type is a file just show that he sent a file
            print_message(sender + " sent a file.");
        }
    }
    sqlite3_finalize(stmt);
}