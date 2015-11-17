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


#include "lib.hpp"
#include "lock.hpp"

#define CONN_MGMT_KEY 8001

#define MAX_UNREAD 10
#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 1024


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
  :fd(-1),
  id(-1),
  pid(-1),
  nick("(no name)")
  {
  }

  bool is_valid() const{
    return fd >= 0;
  }

  void disconnect();
  // send message
  void send_message_by_name(const string& name,const string& msg);

  // send message by id
  void send_message_by_id(int id,const string& msg);
 
  // add message to self
  bool add_message(int from_id,const string& msg){
    bool ret = msg_q.add_message(from_id,msg);
    // notify the thread
    kill(pid,L_SIGMSG);
    return ret;
  }

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
    inet_ntop(AF_INET,&cli_addr->sin_addr,slot.ip,sizeof(slot.ip));
    slot.port = ntohs(cli_addr->sin_port);
    slot.fd = fd;
    slot.pid = getpid();
#ifdef DEBUG
    cerr << "[Server] New client from: " << slot.ip << "/" << slot.port << ", client id = " << slot.id << endl;
#endif
    stringstream ss;
    ss << "*** User '(no name)' entered from (" << slot.ip << '/' << slot.port << "). ***\n";
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
// send message
void ConnClientEntry::send_message_by_name(const string& name,const string& msg){

}

// send message by id
void ConnClientEntry::send_message_by_id(int id,const string& msg){

}
// disconnect
void ConnClientEntry::disconnect(){
    stringstream ss;
    ss << "*** User '" << nick <<"' left. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
    fd = -1;
  }

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
