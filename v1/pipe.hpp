#ifndef __PIPE_HPP_INCLUDED
#define __PIPE_HPP_INCLUDED
#include <string>
#include <deque>
#include <iostream>

#include <unistd.h>


#include "lib.hpp"
#include "fdstream.hpp"

using std::string;
using std::getline;
using std::deque;
using std::cout;
using std::endl;

#define PIPE_MAX_READ 65535


class UnixPipe{
public:
  UnixPipe(bool real = true){
    if(!real){
      read_fd = -1;
      write_fd = -1;
    }
    else{
      int fd[2];
      pipe(fd);
      read_fd = fd[0];
      write_fd = fd[1];
    }
  } 
  bool is_write_closed()const{
    return write_fd < 0;
  }
  bool is_read_closed()const{
    return read_fd < 0;
  }
  int write(const string& str)const{
    if(!is_write_closed()){
      return ::write(write_fd,str.c_str(),str.length()+1);
    }
    else{
      throw "Write end is closed!";
      return 0;
    }
  }
  int writeline(const string& str)const{
    return write(str+"\r\n");
  }
  string getline()const{
    if(!is_read_closed()){
      string str;
      ifdstream is(read_fd);
      ::getline(is,str);
      return str;
    }
    else{
      throw "Read end is closed!";
      return "";
    }
  }
  bool close_read(){
    if(!is_read_closed()){
      ::close(read_fd);
      read_fd = -1;
      return true;
    }
    return false;
  }
  bool close_write(){
    if(!is_write_closed()){
      ::close(write_fd);
      write_fd = -1;
      return true;
    }
    return false;
  }
  bool is_alive() const{
    return !is_write_closed() || !is_read_closed();
  }
  void close(){
    close_read();
    close_write();
  }

  // member variables

  int read_fd,write_fd;

};

#endif
