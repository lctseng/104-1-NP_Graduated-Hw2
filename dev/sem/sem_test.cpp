#include <iostream>
#include <fstream>
#include "lock.hpp"
#include "unistd.h"

#define RUN 100
using namespace std;



void write_n(int n){
  ofstream fout("seqno.txt");
  fout << n;
  fout.close();
}
int read_n(){
  int n;
  ifstream fin("seqno.txt");
  fin >> n;
  fin.close();
  return n;
}

int main(){
  int n = 0;
  write_n(0);
  pid_t pid = fork();
  for(int i=0;i<RUN;i++){
    pipe_lock();
    n = read_n();
    cout << "Pid=" << pid << ",val = " << n << "\n";
    write_n(n+1);
    pipe_unlock();
  }
}
