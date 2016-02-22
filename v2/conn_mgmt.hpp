#ifndef __CONN_MGMT_HPP_INCLUDED
#define __CONN_MGMT_HPP_INCLUDED
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>

//#define DEMO

#include "lib.hpp"
#include "lock.hpp"

#define CONN_MGMT_KEY 8001

#define MAX_UNREAD 10
#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 100


using std::cerr;
using std::endl;
using std::stringstream;

class ConnMgmt;
class ConnClientEntry;

extern ConnClientEntry* client_p;


class ConnMsgEntry{
public:
  ConnMsgEntry()
  :from_id(-1)
  {
    str[0] = '\0';
  }

  void set_message(const string& cpp_str){
    strncpy(str,cpp_str.c_str(),MAX_MSG_LEN);
    str[MAX_MSG_LEN-1] = '\0';
  }

  char str[MAX_MSG_LEN];
  int from_id;
};

class ConnMsgQueue{
public:
  ConnMsgQueue()
  :size(0)
  {
    
  }

  bool add_message(int from_id,const string& str){
    if(size >= 10){
      return false;
    }
    else{
      // set new message
      ConnMsgEntry& ent = msgs[size++];
      ent.from_id = from_id;
      ent.set_message(str);
      return true;
    }
  }

  ConnMsgEntry msgs[MAX_UNREAD];
  int size;
};

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
    pid = -1;
    strcpy(nick,"(no name)");
  }

  void disconnect();
  // send tell
  bool send_tell_message(int tell_id,const string& str); 

  // send yell
  void send_yell_message(const string& msg);

  // add message to self
  bool add_message(int from_id,const string& msg){
    bool ret = msg_q.add_message(from_id,msg);
    // notify the thread
    kill(pid,L_SIGMSG);
    return ret;
  }
  // change name
  bool change_name(const string& name);

  ConnMsgQueue msg_q;
  ConnMgmt* p_mgmt;
  char nick[MAX_NICK_LEN];
  char ip[20];
  unsigned short port;
  int fd;
  int id;
  pid_t pid;
};

class ConnMgmt{
public:
  ConnMgmt()
  :max_id(0)
  {
    for(int i=0;i<MAX_CLIENT;i++){
      clients[i].p_mgmt = this;
      clients[i].id = i+1;
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
    slot.fd = fd;
    slot.pid = getpid();
    cerr << "[Server] New client from: " << slot.ip << "/" << slot.port << ", client id = " << slot.id << endl;
    stringstream ss;
    ss << "*** User '(no name)' entered from " << slot.ip << '/' << slot.port << ". ***\n";
    send_broadcast_message(ss.str());
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
    stringstream ss;
    ss << "*** User '" << nick <<"' left. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
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

// Global Variable

static int shm_id;
ConnMgmt* p_mgmt;

void signal_msg_handler(int sig){
#ifdef DEBUG
  cerr << "[Signal] Reading messages..." << endl;
#endif
  conn_lock();
  ConnMsgQueue& msg_q = client_p->msg_q;
  for(int i=0;i<msg_q.size;i++){
    ConnMsgEntry& msg = msg_q.msgs[i];
    cout << msg.str;
  }
  msg_q.size = 0;
  conn_unlock();
}
void conn_mgmt_init(){
  // create shm
  shm_id = shmget(CONN_MGMT_KEY,sizeof(ConnMgmt),0600 |IPC_CREAT);
  if(shm_id < 0){
    debug("Error when get shm!");
    debug(errno);
    exit(1);
  }
  p_mgmt = (ConnMgmt*)shmat(shm_id,0,0);
  if((long)p_mgmt == -1){
    debug("Error when attach!");
    exit(1);
  }
  // init data
  new (p_mgmt) ConnMgmt;
  // set message handler
  signal(L_SIGMSG,signal_msg_handler);
}
void conn_mgmt_cleanup(){
  shmdt(p_mgmt);
  shmctl(shm_id,IPC_RMID,0);
}



#endif
