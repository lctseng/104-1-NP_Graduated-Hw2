#ifndef __CONN_MGMT_HPP_INCLUDED
#define __CONN_MGMT_HPP_INCLUDED
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define CONN_MGMT_KEY 8001

#define MAX_UNREAD 10
#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 1024


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

  bool isValid() const{
    return fd >= 0;
  }

  ConnMsgQueue msg_q;
  char nick[MAX_NICK_LEN];
  int fd;
  int id;
};

class ConnMgmt{
public:
  ConnMgmt()
  :max_id(-1)
  {
    for(int i=0;i<MAX_CLIENT;i++){
      clients[i].id = i;
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
