#ifndef __CONN_MGMT_HPP_INCLUDED
#define __CONN_MGMT_HPP_INCLUDED
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>

#include "lib.hpp"

#define CONN_MGMT_KEY 8001

#define MAX_UNREAD 10
#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 1024


using std::cerr;
using std::endl;

class ConnMsgEntry{
public:
  ConnMsgEntry()
  :from_id(-1)
  {
    str[0] = '\0';
  }
  char str[MAX_MSG_LEN];
  int from_id;
};

class ConnMsgQueue{
public:
  ConnMsgEntry msgs[MAX_UNREAD];
};

class ConnClientEntry{
public:
  ConnClientEntry()
  :fd(-1),
  id(-1)
  {
    nick[0] = '\0';
  }

  bool is_valid() const{
    return fd >= 0;
  }

  void disconnect(){
#ifdef DEBUG
    cerr << "Client " << id << " from " << ip << "/" << port << " disconnected" << endl;
#endif
    fd = -1;
  }

  ConnMsgQueue msg_q;
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
    for(int i=0;i<MAX_CLIENT;i++){
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
    // debug
#ifdef DEBUG
    cerr << "[Server] New client from: " << slot.ip << "/" << slot.port << ", client id = " << slot.id << endl;
#endif
    return slot;
  }
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


static int shm_id;
ConnMgmt* p_mgmt;

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
}
void conn_mgmt_cleanup(){
  shmdt(p_mgmt);
  shmctl(shm_id,IPC_RMID,0);
}


#endif
