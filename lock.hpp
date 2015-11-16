#ifndef __LOCK_HPP_INCLUDED
#define __LOCK_HPP_INCLUDED

#include "semaphore.hpp"
#include <iostream>



#define SEMKEY_CONNECT ((key_t)7890) // for connection&chat
#define SEMKEY_PIPE ((key_t)7891) // for pub pipe

int sem_conn = -1;
int sem_pipe = -1;

void lock_init(){
  
}
void lock_clean_up(){
  sem_close(sem_conn);
  sem_close(sem_pipe);
}

// conn lock
void conn_lock()
{
  if (sem_conn < 0) {
    if ( (sem_conn = sem_create(SEMKEY_CONNECT, 1)) < 0) err_sys("sem_create error for connection lock");
  }
  sem_wait(sem_conn);
}
void conn_unlock()
{
  sem_signal(sem_conn);
}
// pub pipe lock
void pipe_lock()
{
  if (sem_pipe < 0) {
    if ( (sem_pipe = sem_create(SEMKEY_PIPE, 1)) < 0) err_sys("sem_create error for pipe lock");
  }
  sem_wait(sem_pipe);
}
void pipe_unlock()
{
  sem_signal(sem_pipe);
}



#endif
