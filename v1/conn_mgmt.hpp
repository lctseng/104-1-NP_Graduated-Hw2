#ifndef __CONN_MGMT_HPP_INCLUDED
#define __CONN_MGMT_HPP_INCLUDED
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>

#define DEMO

#include "lib.hpp"
#include "fdstream.hpp"

#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 100


using std::cerr;
using std::endl;
using std::stringstream;

class ConnMgmt;
class ConnClientEntry;
bool run_command(ConnClientEntry& client,const string& str);

class ConnClientEntry{
public:

  ConnClientEntry()
  :id(-1)
  {
    reset_entry();
  }

  bool is_valid() const{
    return fd >= 0;
  }
  
  void reset_entry(){
    fd = -1;
    strcpy(nick,"(no name)");
    p_fin = nullptr;
    p_fout = nullptr;
  }

  void disconnect();
  // send tell
  bool send_tell_message(int tell_id,const string& str); 

  // send yell
  void send_yell_message(const string& msg);

  // add message to self
  bool add_message(int from_id,const string& msg){
    *p_fout << msg;
    p_fout->flush();
    return true;
  }
  // change name
  bool change_name(const string& name);

  void init_client(int new_fd){
    fd = new_fd;
    p_fin = new ifdstream(fd);
    p_fout = new ofdstream(fd);
  }

  // process client
  // return false for EOF
  bool process_client(){
    // read a line from client
    string str;
    if(getline(*p_fin,str)){
      if(run_command(*this,str)){
        (*p_fout) << "% ";
        p_fout->flush();
        return true;
      } 
      else{
        // client exit
        return false;
      }
      return true;
    }
    else{
      // EOF
      return false;
    }
  }

  ConnMgmt* p_mgmt;
  ifdstream *p_fin;
  ofdstream *p_fout;
  char nick[MAX_NICK_LEN];
  char ip[20];
  unsigned short port;
  int fd;
  int id;
};

class ConnMgmt{
public:
  ConnMgmt()
  :max_id(0)
  {
    cout << "Mgmt reset" << endl;
    for(int i=0;i<MAX_CLIENT;i++){
      clients[i].p_mgmt = this;
      clients[i].id = i+1;
    }
    
  }
  void disconnect_all(){
    for(ConnClientEntry& client : clients){
      if(client.is_valid() ){
        client.disconnect();
      }
    }
  }

  // add new client
  ConnClientEntry& register_new_client(int fd,sockaddr_in* cli_addr){
    // search empty slot
    ConnClientEntry& slot = find_empty_slot();
    // fill info
#ifndef DEMO
    inet_ntop(AF_INET,&cli_addr->sin_addr,slot.ip,sizeof(slot.ip));
    slot.port = ntohs(cli_addr->sin_port);
#else
    strncpy(slot.ip,"CGILAB",sizeof(slot.ip));
    slot.port = 511;
#endif
    slot.init_client(fd);
#ifdef DEBUG
    cerr << "[Server] New client from: " << slot.ip << "/" << slot.port << ", client id = " << slot.id << ", fd = " << fd << endl;
#endif
    // send welcome
    (*slot.p_fout) <<  "****************************************" << endl
    << "** Welcome to the information server. **" << endl
    << "****************************************" << endl;
    // send notify
    stringstream ss;
    ss << "*** User '(no name)' entered from " << slot.ip << '/' << slot.port << ". ***\n";
    send_broadcast_message(ss.str());
    (*slot.p_fout) << "% ";
    slot.p_fout->flush();
    return slot;
  }
  // send broadcast message
  void send_broadcast_message(const string& msg){
    for(ConnClientEntry& ent : clients){
      if(ent.is_valid()){
        ent.add_message(-1,msg);
      }
    }
  }
  // check name exist in list
  bool check_name_exist(const string& name){
    for(int i=0;i<=max_id;i++){
      if(clients[i].is_valid()){
        if(name == clients[i].nick){
          return true;
        }
      }   
    }
    return false;
  }
  //-- internal
  // find empty slot
  ConnClientEntry& find_empty_slot(){
    for(int i=0;i<=max_id;i++){
      if(!clients[i].is_valid()){
        max_id = std::max(max_id,i+1);
        return clients[i];
      }   
    }
  }


  int max_id;
  ConnClientEntry clients[MAX_CLIENT];
};

// For ConnClientEntry
// disconnect
void ConnClientEntry::disconnect(){
    cerr << "[Server] Disconnecting client #" << id << " from " << ip << "/" << port << " (fd = " << fd << ")" << endl;
    stringstream ss;
    ss << "*** User '" << nick <<"' left. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
    delete p_fin;
    delete p_fout;
    reset_entry();
}
// send tell
bool ConnClientEntry::send_tell_message(int tell_id,const string& msg){
  // search for id
  // check range
  if(tell_id < 1 || tell_id > MAX_CLIENT){
    return false;
  }
  ConnClientEntry& ent = p_mgmt->clients[tell_id-1];
  if(ent.is_valid()){
    stringstream ss;
    ss << "*** " << nick <<" told you ***: " << msg << "\n";
    ent.add_message(id,ss.str());
    return true;
  }
  else{
    // target client not existed
    return false;
  }
}
// change name
bool ConnClientEntry::change_name(const string& name){
  if(p_mgmt->check_name_exist(name)){
    return false; // name exists
  }
  else{
    // change it
    strncpy(nick,name.c_str(),MAX_NICK_LEN-1);
    nick[MAX_NICK_LEN-1] = '\0';
    // broadcast
    stringstream ss;
    ss << "*** User from " << ip << '/' << port << " is named '" << nick << "'. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
    
    return true;
  }
}
// send yell
void ConnClientEntry::send_yell_message(const string& msg){
    stringstream ss;
    ss << "*** " << nick <<" yelled ***: " << msg << "\n";
    const string& send_msg = ss.str();
    for(ConnClientEntry& ent : p_mgmt->clients){
      if(ent.is_valid()){
        ent.add_message(-1,send_msg);
      }
    }
}


// variables
ConnMgmt conn_mgmt;
#endif