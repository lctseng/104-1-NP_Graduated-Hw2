#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;

int main(int argc,char** argv){

  int fd = open("TEST",1 | O_NONBLOCK);
  char buf[100];
  int i=0;
  if(argc > 1){
    while(1){
      i++;
      sleep(1);
      sprintf(buf,"%d\n",i);
      write(fd,buf,strlen(buf));
    } 
  }
  else{
    sleep(100); 
  }
}
