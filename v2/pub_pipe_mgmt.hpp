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
#define PIPE_PERM 0600

class PubPipeMgmt;
class PubPipeEntry{
public:
  
  PubPipeEntry():valid(false){}

  
  void clear(bool signal = true);
  int open_write();
  int open_read();
  bool create();
  // fixed data
  PubPipeMgmt* p_mgmt;
  char name[PIPE_NAME_LEN];
  int id;
  // dynamic data
  bool valid;
  int fake_read_fd;
};

class PubPipeMgmt{
public:
  PubPipeMgmt(){
    pipe_to_open = -1;
    pipe_to_close = -1;
    for(int i=0;i<MAX_PUB_PIPE;i++){
      pipes[i].p_mgmt = this;
      pipes[i].id = i;
      std::snprintf(pipes[i].name,PIPE_NAME_LEN,"/tmp/NP_HW2_V2_PIPE_0116057_%d",i);
      pipes[i].clear();
    }
  }

  // call by master
  void clear_all(){
    pipe_lock();
    pipe_to_open = -1;
    for(PubPipeEntry& ent : pipes){
      if(ent.valid){
        close(ent.fake_read_fd);
        ent.clear(false);
      }
    }
    pipe_unlock();
  }
  int open_write(int pipe_id){
    // check range
    if(pipe_id < 0 || pipe_id >= MAX_PUB_PIPE){
      return -1;
    }
    return pipes[pipe_id].open_write();
  }
  int open_read(int pipe_id){
    // check range
    if(pipe_id < 0 || pipe_id >= MAX_PUB_PIPE){
      return -1;
    }
    return pipes[pipe_id].open_read();
  }
  void clear_pipe(int pipe_id){
    // check range
    if(pipe_id < 0 || pipe_id >= MAX_PUB_PIPE){
      return ;
    }
    pipes[pipe_id].clear(); // with signal
  }
  
    
  int pipe_to_open;
  int pipe_to_close;
  pid_t master_pid;
  PubPipeEntry pipes[MAX_PUB_PIPE];
};

void PubPipeEntry::clear(bool signal){
  if(valid){
    // pipe was active, remove it and deactivate it
    valid = false;
    if(signal){
      p_mgmt->pipe_to_close = id;
      kill(p_mgmt->master_pid,L_SIGPIPE);
    }
    else{
      fake_read_fd = -1;
    }
    unlink(name);
  }
}

// create by writer, return the fd opened for write, -1 for error
int PubPipeEntry::open_write(){
  if(create()){
    // fifo created 
    // open fake reader
    p_mgmt->pipe_to_open = id;
    kill(p_mgmt->master_pid,L_SIGPIPE);
    // open a write end fd 
    return open(name, 1 );
  }
  else{
    // create failed
    return -1;
  }
}
int PubPipeEntry::open_read(){
  if(valid){
    // open for read
    return open(name,0 | O_NONBLOCK);
  }
  else{
    return -1;
  }
  if(create()){
    // fifo created 
    // open fake reader
    p_mgmt->pipe_to_open = id;
    kill(p_mgmt->master_pid,L_SIGPIPE);
    // open a read end fd 
    return open(name, 1 );
  }
  else{
    // create failed
    return -1;
  }
}


bool PubPipeEntry::create(){
  if(valid){
    // already existed
    return false;
  }
  if ( (mknod(name, S_IFIFO | PIPE_PERM, 0) < 0) && (errno != EEXIST)) {
    unlink(name);
    debug("FIFO cannot be created!");
    return false;
  }
  valid = true;
  return true;
}
namespace PubPipe{
  int shm_id;
  PubPipeMgmt* p_mgmt;
};



void pub_pipe_mgmt_cleanup(){
  PubPipe::p_mgmt->clear_all();
  shmdt(PubPipe::p_mgmt);
  shmctl(PubPipe::shm_id,IPC_RMID,0);
  
}
void signal_pipe_handler(int sig){
  
  int open_id = PubPipe::p_mgmt->pipe_to_open;
  int close_id = PubPipe::p_mgmt->pipe_to_close;
  if(open_id>=0){
    cerr << "[Server] Open pipe for id :" << open_id << endl;
    PubPipeEntry& ent = PubPipe::p_mgmt->pipes[open_id];
    ent.fake_read_fd = open(ent.name,0 | O_NONBLOCK);
    PubPipe::p_mgmt->pipe_to_open = -1;
  }
  if(close_id>=0){
    cerr << "[Server] Close pipe for id :" << close_id << endl;
    PubPipeEntry& ent = PubPipe::p_mgmt->pipes[close_id];
    close(ent.fake_read_fd);
    ent.fake_read_fd = -1;
    PubPipe::p_mgmt->pipe_to_close = -1;
  }
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
  PubPipe::p_mgmt->master_pid = getpid();
  // set pipe handler
  signal(L_SIGPIPE,signal_pipe_handler);
  
}



#endif
