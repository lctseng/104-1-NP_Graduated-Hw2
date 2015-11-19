#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace std;

int main(){

  mknod("TEST", S_IFIFO | 0666, 0);
  int fd = open("TEST",1 | O_NONBLOCK);
  char buf[100];
  int i=0;
  while(1){
    i++;
    sleep(1);
    sprintf(buf,"%d\n",i);
    write(fd,buf,strlen(buf));
  }
}
