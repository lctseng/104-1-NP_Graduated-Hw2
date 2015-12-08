
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
  int fd = open("TEST",0 | O_NONBLOCK);
  cout << "Opened" << endl;
  sleep(100);
  return 0;
}
