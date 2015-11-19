
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

  int fd = open("TEST",0 | O_NONBLOCK);
  sleep(10);
  return 0;
}
