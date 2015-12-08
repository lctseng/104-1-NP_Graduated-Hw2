#ifndef __CONN_MGMT_HPP_INCLUDED
#define __CONN_MGMT_HPP_INCLUDED
#include <iostream>
#include <sstream>
#include <vector>
#include <deque>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>


#define DEMO
#define ERREXIST 1
#define ERRDUP 2

#include "lib.hpp"
#include "fdstream.hpp"

#define MAX_CLIENT 40
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 100


using std::cerr;
using std::endl;
using std::stringstream;
using std::vector;
using std::deque;

class ConnMgmt;
class ConnClientEntry;
bool run_command(ConnClientEntry& client,const string& str);


class EnvVariable{
public:
  EnvVariable(const string& key,const string& value = "")
  :key(key),
  value(value)
  {
  }

  void apply(){
    setenv(key.c_str(),value.c_str(),1); 
  }

  string key;
  string value;
};

class EnvVariables{
public:

  EnvVariables(){
    reset();
  }

  string get_value(const string& key){
    for(EnvVariable& env: data){
      if(env.key == key){
        return env.value;
      } 
    }
    return "";
  }

  void set_value(const string& key,const string& value){
    bool found = false;
    for(EnvVariable& env: data){
      if(env.key == key){
        env.value = value;
        found = true;
      } 
    }
    if(!found){
      data.emplace_back(key,value);
    }
    
  }

  void apply_all(){
    clearenv();
    for(EnvVariable& env: data){
      env.apply();
    }
  }

  void reset(){
    data.clear();
    // basic env: PATH
    data.emplace_back("PATH","bin:.");
  }

  vector<EnvVariable> data;
};



class ConnClientEntry{
public:

  ConnClientEntry()
  :id(-1)
  {
    reset_entry();
  }

  bool is_valid() const{
    return fd >= 0;
  }
  
  void reset_entry(){
    fd = -1;
    strcpy(nick,"(no name)");
    p_fin = nullptr;
    p_fout = nullptr;
    change_count = 0;
  }

  void disconnect();
  // send tell
  bool send_tell_message(int tell_id,const string& str); 

  // send yell
  void send_yell_message(const string& msg);

  // add message to self
  bool add_message(int from_id,const string& msg){
    *p_fout << msg;
    p_fout->flush();
    return true;
  }
  // change name
  int change_name(const string& name);

  void init_client(int new_fd){
    fd = new_fd;
    p_fin = new ifdstream(fd);
    p_fout = new ofdstream(fd);
    reset_pipe_pool();
    env.reset();
  }

  // process client
  // return false for EOF
  bool process_client(){
    // read a line from client
    string str;
    if(getline(*p_fin,str)){
      if(run_command(*this,str)){
        (*p_fout) << "% ";
        p_fout->flush();
        return true;
      } 
      else{
        // client exit
        return false;
      }
      return true;
    }
    else{
      // EOF
      return false;
    }
  }


  void reset_pipe_pool(){
    pipe_pool.clear();
    pipe_pool.push_back(UnixPipe(false));
  }

  UnixPipe create_numbered_pipe(int count){
    int current_max = (int)pipe_pool.size() - 1;
    if(current_max < count){ // new pipe needed
      return UnixPipe();
    }
    else{
      // check of that pipe is alive
      if(pipe_pool[count].is_alive()){
        return pipe_pool[count];
      }
      else{
        return UnixPipe();
      }
    }
  }
  void record_pipe_info(int count,const UnixPipe& pipe){
    if(count >= 0){
      int current_max = (int)pipe_pool.size() - 1;
      if(current_max < count){
        // fill pipe pool
        for(int i=current_max+1;i<count;i++){

          pipe_pool.push_back(UnixPipe(false));
        }
        // create on that
        pipe_pool.push_back(pipe);
      }
      else{
        // check of that pipe is alive
        if(!pipe_pool[count].is_alive()){
          pipe_pool[count] = pipe;
        }
      }
    }
  }

  void close_pipe_larger_than(int count){
    if(pipe_pool.size()>1){
      for(int i=count+1;i<pipe_pool.size();i++){
        pipe_pool[i].close();
      }
    }
  }

  void decrease_pipe_counter_and_close(){
    if(!pipe_pool.empty()){
      pipe_pool.pop_back();
      if(!pipe_pool.empty()){
        pipe_pool[0].close();
      }
    }
  }





  // Variables
  deque<UnixPipe> pipe_pool;

  ConnMgmt* p_mgmt;
  ifdstream *p_fin;
  ofdstream *p_fout;
  EnvVariables env;
  int change_count;
  char nick[MAX_NICK_LEN];
  char ip[20];
  unsigned short port;
  int fd;
  int id;
};

class ConnMgmt{
public:
  ConnMgmt()
  :max_id(0)
  {
    cout << "Mgmt reset" << endl;
    for(int i=0;i<MAX_CLIENT;i++){
      clients[i].p_mgmt = this;
      clients[i].id = i+1;
    }
    
  }
  void disconnect_all(){
    for(ConnClientEntry& client : clients){
      if(client.is_valid() ){
        client.disconnect();
      }
    }
  }

  // add new client
  ConnClientEntry& register_new_client(int fd,sockaddr_in* cli_addr){
    // search empty slot
    ConnClientEntry& slot = find_empty_slot();
    // fill info
#ifndef DEMO
    inet_ntop(AF_INET,&cli_addr->sin_addr,slot.ip,sizeof(slot.ip));
    slot.port = ntohs(cli_addr->sin_port);
#else
    strncpy(slot.ip,"CGILAB",sizeof(slot.ip));
    slot.port = 511;
#endif
    slot.init_client(fd);
#ifdef DEBUG
    cerr << "[Server] New client from: " << slot.ip << "/" << slot.port << ", client id = " << slot.id << ", fd = " << fd << endl;
#endif
    // send welcome
    (*slot.p_fout) <<  "****************************************" << endl
    << "** Welcome to the information server. **" << endl
    << "****************************************" << endl;
    // send notify
    stringstream ss;
    ss << "*** User '(no name)' entered from " << slot.ip << '/' << slot.port << ". ***\n";
    send_broadcast_message(ss.str());
    (*slot.p_fout) << "% ";
    slot.p_fout->flush();
    return slot;
  }
  // send broadcast message
  void send_broadcast_message(const string& msg){
    for(ConnClientEntry& ent : clients){
      if(ent.is_valid()){
        ent.add_message(-1,msg);
      }
    }
  }
  // check name exist in list
  bool check_name_exist(const string& name){
    for(int i=0;i<=max_id;i++){
      if(clients[i].is_valid()){
        if(name == clients[i].nick){
          return true;
        }
      }   
    }
    return false;
  }
  //-- internal
  // find empty slot
  ConnClientEntry& find_empty_slot(){
    for(int i=0;i<=max_id;i++){
      if(!clients[i].is_valid()){
        max_id = std::max(max_id,i+1);
        return clients[i];
      }   
    }
  }


  int max_id;
  ConnClientEntry clients[MAX_CLIENT];
};

// For ConnClientEntry
// disconnect
void ConnClientEntry::disconnect(){
    cerr << "[Server] Disconnecting client #" << id << " from " << ip << "/" << port << " (fd = " << fd << ")" << endl;
    stringstream ss;
    ss << "*** User '" << nick <<"' left. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
    delete p_fin;
    delete p_fout;
    reset_entry();
}
// send tell
bool ConnClientEntry::send_tell_message(int tell_id,const string& msg){
  // search for id
  // check range
  if(tell_id < 1 || tell_id > MAX_CLIENT){
    return false;
  }
  ConnClientEntry& ent = p_mgmt->clients[tell_id-1];
  if(ent.is_valid()){
    stringstream ss;
    ss << "*** " << nick <<" told you ***: " << msg << "\n";
    ent.add_message(id,ss.str());
    return true;
  }
  else{
    // target client not existed
    return false;
  }
}
// change name
int ConnClientEntry::change_name(const string& name){
  if(p_mgmt->check_name_exist(name)){
    return ERREXIST; // name exists
  }
  else if(change_count >= 3){
    return ERRDUP;
  }
  else{
    // change it
    strncpy(nick,name.c_str(),MAX_NICK_LEN-1);
    nick[MAX_NICK_LEN-1] = '\0';
    // broadcast
    stringstream ss;
    ss << "*** User from " << ip << '/' << port << " is named '" << nick << "'. ***\n";
    p_mgmt->send_broadcast_message(ss.str());
    ++change_count; 
    return 0;
  }
}
// send yell
void ConnClientEntry::send_yell_message(const string& msg){
    stringstream ss;
    ss << "*** " << nick <<" yelled ***: " << msg << "\n";
    const string& send_msg = ss.str();
    for(ConnClientEntry& ent : p_mgmt->clients){
      if(ent.is_valid()){
        ent.add_message(-1,send_msg);
      }
    }
}


// variables
ConnMgmt conn_mgmt;
#endif
