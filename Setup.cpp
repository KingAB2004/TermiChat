#include<iostream>
#include<sqlite3.h>
#include<filesystem>
#include<cstdlib>

namespace fs =std::filesystem;

int main(){
     char * home = getenv("HOME");
     if(!home)
     {
        std::cerr <<"Error: HOME not there change the address in the file to make it work\n";
        return 1;
     }  


   //   Creating the TermiChat folder in the Public Directory along with ChatFiles Folder where the Files Transfered would be Stored
     fs ::path dir =fs::path(home)/"Public"/"TermiChat";
     fs ::path dirFile =fs::path(home)/"Public"/"TermiChat/ChatFiles";
     fs ::path db_path = dir /"friend.db";

     try{
        if(!fs :: exists(dir)){
            fs :: create_directory(dir);
        }
        if(!fs :: exists(dirFile)){
            fs :: create_directory(dir);
        }
     }catch(fs::filesystem_error&e){
        std::cerr << e.what()<<"\n";
     }



     sqlite3 * frienddb;
   // Creating the friends.db for Storing the names of the friends along with their names and their previous Messages they had sent
     if(sqlite3_open(db_path.c_str() ,&frienddb))
     {
        std::cerr << "Error creating the Database\n";
        return 1;
     }


   //   Creating the friends and the message table
    const char* create_table ="create table if not exists friends(id INTEGER PRIMARY KEY AUTOINCREMENT ,name text NOT NULL ,ip text not NULL unique)";
   const char* create_Message_Table ="CREATE TABLE IF NOT EXISTS messages( id INTEGER PRIMARY KEY AUTOINCREMENT,friend_name TEXT,sender TEXT,type TEXT,content TEXT,timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
     if(sqlite3_exec(frienddb ,create_table ,0 ,0 ,nullptr)!=SQLITE_OK)
     {
        std:: cerr << "Error executing the create table commmand\n";
     }
      if(sqlite3_exec(frienddb ,create_Message_Table ,0 ,0 ,nullptr)!=SQLITE_OK)
     {
        std:: cerr << "Error executing the create table commmand\n";
     }
     

     
     sqlite3_close(frienddb);
}
