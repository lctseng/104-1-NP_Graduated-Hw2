#include <iostream>
#include <cstdlib>
#include <regex>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "fdstream.hpp"
#include "lib.hpp"
#include "conn_mgmt.hpp"
//#include "shell.hpp"
//#include "lock.hpp"
//#include "pub_pipe_mgmt.hpp"


#define DEFAULT_PORT 16057
#define BASE_DIR "ras"


using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::string;
using std::atoi;
using std::getline;


// Globals
Sigfunc old_intr_handler;
ConnMgmt conn_mgmt;

// cleanup, include all ipc-related item 
// should called by master thread only once
void clean_up(){
  cerr << "Cleaning up..." << endl;
  conn_mgmt.disconnect_all();
}


// Signal handler for SIGINT
void handle_sigint(int sig) {
  clean_up();
  cerr << "Exiting..." << endl;
  conn_mgmt.~ConnMgmt();
  exit(1);
}

void register_sigint(){
  old_intr_handler = signal(SIGINT,handle_sigint);
}
void restore_sigint(){
  signal(SIGINT,old_intr_handler);
}
int main(int argc,char** argv){
  conn_mgmt = ConnMgmt();

  register_sigint();
  // change to base path
  chdir(BASE_DIR);
  // get port
  int port;
  if(argc < 2){
    port = DEFAULT_PORT; 
  }
  else{
    port = atoi(argv[1]);
  }
  cout << "Listen to port:" << port << endl;
  // start program
  int sockfd,clientfd;
  socklen_t cli_addr_len;
  sockaddr_in cli_addr,serv_addr;
  
  // socket
  if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
    err_abort("can't create server socket");
  }
  // fill addr
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  // bind
  if(bind(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0){
    err_abort("can't bind local address");
  } 
  // listen
  listen(sockfd,0);
  // fdset init for select!
  fd_set all_set;
  fd_set r_set;
  int max_fd = getdtablesize();
  FD_ZERO(&all_set);
  FD_SET(sockfd,&all_set);
  // main loop
  while(true){
    r_set = all_set;
    if(select(max_fd,&r_set,nullptr,nullptr,nullptr)<0){
      cerr << "Error for select! errno=" << errno << endl;
      clean_up();
      exit(1);
    }
    // master sock
    if(FD_ISSET(sockfd,&r_set)){
      cli_addr_len = sizeof(cli_addr);
      clientfd = accept(sockfd,(sockaddr*)&cli_addr,&cli_addr_len);
      if(clientfd<0){
        debug("Accept error!");
      }
      FD_SET(clientfd,&all_set);
      conn_mgmt.register_new_client(clientfd,&cli_addr);
    }
    // client sock
    for(ConnClientEntry& client : conn_mgmt.clients){
      if(client.is_valid() && FD_ISSET(client.fd,&r_set)){
        if(!client.process_client()){
          // disconnect
          client.disconnect();
          FD_CLR(client.fd,&all_set);
        }
      } 
    }
  }

}

