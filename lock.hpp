#ifndef __LOCK_HPP_INCLUDED
#define __LOCK_HPP_INCLUDED

#include "semaphore.hpp"
#include "lib.hpp"

#include <iostream>
#include <signal.h>


#define SEMKEY_CONNECT ((key_t)7890) // for connection&chat
#define SEMKEY_PIPE ((key_t)7891) // for pub pipe


int sem_conn = -1;
int sem_pipe = -1;

static sigset_t signal_set,old_signal_set; 
static bool allow_msg;


void lock_init(){
  if (sem_conn < 0) {
    if ( (sem_conn = sem_create(SEMKEY_CONNECT, 1)) < 0) err_sys("sem_create error for connection lock");
  }
  if (sem_pipe < 0) {
    if ( (sem_pipe = sem_create(SEMKEY_PIPE, 1)) < 0) err_sys("sem_create error for pipe lock");
  }
  // init sig set
  sigemptyset(&signal_set);
  sigaddset(&signal_set, L_SIGMSG);
  allow_msg = false;
  
}
void lock_clean_up(){
  sem_close(sem_conn);
  sem_close(sem_pipe);
}

void block_sig_msg(){
  allow_msg = false;
}
void unblock_sig_msg(){
  allow_msg = true;
  sigprocmask(SIG_UNBLOCK, &signal_set, &old_signal_set);
}

// conn lock
void conn_lock()
{
  if (sem_conn < 0) {
    if ( (sem_conn = sem_create(SEMKEY_CONNECT, 1)) < 0) err_sys("sem_create error for connection lock");
  }
  sem_wait(sem_conn);
  // block msg signal
  sigprocmask(SIG_BLOCK, &signal_set, &old_signal_set);

}
void conn_unlock()
{
  sem_signal(sem_conn);
  // unblock signal
  if(allow_msg)
    sigprocmask(SIG_UNBLOCK, &signal_set, &old_signal_set);
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
