#include <iostream>
#include <cstdlib>
#include <regex>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "lib.hpp"
#include "shell.hpp"

//#define CONSOLE

#define DEFAULT_PORT 16057
#define BASE_DIR "ras"


using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::atoi;
using std::getline;

int main(int argc,char** argv){
  // change to base path
  chdir(BASE_DIR);
#ifndef CONSOLE
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
  register_sigchld();
  int sockfd,clientfd;
  socklen_t cli_addr_len;
  sockaddr_in cli_addr,serv_addr;
  pid_t pid;
  
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
  // accept client
  while(true){
    cli_addr_len = sizeof(cli_addr);
    clientfd = accept(sockfd,(sockaddr*)&cli_addr,&cli_addr_len);
    if(clientfd<0){
      debug("Accept error!");
    }
    if((pid=fork())<0){
      err_abort("Fork error!");
    }
    else if(pid>0){
      // parent
      close(clientfd);
    }
    else{
      // child
      restore_sigchld();
      close(sockfd); 
      try{
      run_shell(clientfd,clientfd,clientfd);
      }catch(std::regex_error e){
        debug(e.code()== std::regex_constants::error_escape);
      }
      exit(0);
    }
  }
#else
  run_shell();
#endif
}

