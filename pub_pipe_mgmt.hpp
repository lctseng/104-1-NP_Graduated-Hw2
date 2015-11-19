#ifndef __PUB_PIPE_MGMT_HPP_INCLUDED
#define __PUB_PIPE_MGMT_HPP_INCLUDED

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <cstring>

#include "lib.hpp"

#define PUB_PIPE_MGMT_KEY 8002
#define MAX_PUB_PIPE 101
#define PIPE_NAME_LEN 100


class PubPipeMgmt;
class PubPipeEntry{
public:
  
  PubPipeEntry():valid(false){}

  void clear(){
    if(valid){
      // pipe was active, remove it and deactivate it
      unlink(name);
      valid = false; 
    }
  }

  // create by writer, return the fd opened for write, -1 for error
  int open_write(){
    if(create()){
      // fifo created 
      // open a write end fd 
      return open(name, 1);
    }
    else{
      // create failed
      return -1;
    }
  }
  
  bool create(){
    if(valid){
      // already existed
      return false;
    }
    if ( (mknod(name, S_IFIFO | 0600, 0) < 0) && (errno != EEXIST)) {
      unlink(name);
      debug("FIFO cannot be created!");
      return false;
    }
    valid = true;
    return true;
  }

  // fixed data
  PubPipeMgmt* p_mgmt;
  char name[PIPE_NAME_LEN];
  int id;
  // dynamic data
  bool valid;
};

class PubPipeMgmt{
public:
  PubPipeMgmt(){
    for(int i=0;i<MAX_PUB_PIPE;i++){
      pipes[i].p_mgmt = this;
      pipes[i].id = i;
      std::snprintf(pipes[i].name,PIPE_NAME_LEN,"/tmp/NP_HW2_PIPE_0116057_%d",i);
      pipes[i].clear();
    }
  }
  void clear_all(){
    for(PubPipeEntry& ent : pipes){
      ent.clear();
    }
  }


  PubPipeEntry pipes[MAX_PUB_PIPE];
};

namespace PubPipe{
  int shm_id;
  PubPipeMgmt* p_mgmt;
};



void pub_pipe_mgmt_cleanup(){
  PubPipe::p_mgmt->clear_all();
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
