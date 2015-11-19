#ifndef __PUB_PIPE_MGMT_HPP_INCLUDED
#define __PUB_PIPE_MGMT_HPP_INCLUDED

#include <unistd.h>
#include "lib.hpp"

#define PUB_PIPE_MGMT_KEY 8002


class PubPipeMgmt{
public:
  
};

namespace PubPipe{
  int shm_id;
  PubPipeMgmt* p_mgmt;
};




void pub_pipe_mgmt_cleanup(){
  shmdt(PubPipe::p_mgmt);
  shmctl(PubPipe::shm_id,IPC_RMID,0);
  
}
void pub_pipe_mgmt_init(){
  // create shm
  PubPipe::shm_id = shmget(PUB_PIPE_MGMT_KEY,sizeof(PubPipeMgmt),0600 |IPC_CREAT);
  if(PubPipe::shm_id < 0){
    debug("Error when get shm!");
    debug(errno);
    exit(1);
  }
  PubPipe::p_mgmt = (PubPipeMgmt*)shmat(PubPipe::shm_id,0,0);
  if((long)PubPipe::p_mgmt == -1){
    debug("Error when attach!");
    exit(1);
  }
  // init data
  new (PubPipe::p_mgmt) PubPipeMgmt;
  
}



#endif
