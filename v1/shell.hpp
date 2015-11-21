#ifndef __SHELL_HPP_INCLUDED
#define __SHELL_HPP_INCLUDED
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>

#include "lib.hpp"
#include "conn_mgmt.hpp"
//#include "pub_pipe_mgmt.hpp"

using std::getline;
using std::string;
using std::cout;
using std::cin;
using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;
using std::getenv;

extern ConnMgmt conn_mgmt;

bool run_command(ConnClientEntry& client,const string& src_str){
  string str = string_strip(src_str);
  smatch match;
  if(regex_search(str, regex("^\\s*exit(\\s+|$)"))){
    return false;
  }
  else if(regex_search(str, regex("/"))){
    *client.p_fout << "/ is forbidden here!" << endl;
  }
  // who
  else if(regex_search(str, regex("^\\s*who(\\s+|$)"))){
    *client.p_fout << "<ID>\t<nickname>\t<IP/port>\t<indicate me>" << endl;
    for(ConnClientEntry& ent : conn_mgmt.clients){
      if(ent.is_valid()){
        *client.p_fout << ent.id << '\t' << ent.nick << '\t' << ent.ip << '/' << ent.port << '\t';
        if(&ent == &client){
          // me
          *client.p_fout << "<-me";
        }
        *client.p_fout << endl;
      }
    }
  }
  // change name
  else if(regex_search(str,match,regex("^\\s*name(\\s+|$)"))){
    string new_name = string_strip(match.suffix());
    if(!new_name.empty()){
      if(client.change_name(new_name)){
        // already broadcasted 
      }
      else{
        *client.p_fout << "*** User '" << new_name <<"' already exists. ***" << endl;
      }
    }
    else{
      *client.p_fout << "*** Cannot use an empty name ***" << endl;
    }
  }
  
  // messaging
  // tell
  else if(regex_search(str,match,regex("^\\s*tell\\s+(\\d+)(\\s|$)"))){
    int tell_id = std::stoi(match[1]);
    string msg = string_strip(match.suffix());
    if(!msg.empty()){
      if(!client.send_tell_message(tell_id,msg)){
        // failure
        *client.p_fout << "*** Error: user #" << match[1] <<" does not exist yet. ***" << endl;
      }
    }
  }
  // yell, not yell to self
  else if(regex_search(str,match,regex("^\\s*yell(\\s|$)"))){
    string msg = string_strip(match.suffix());
    if(!msg.empty()){
      client.send_yell_message(msg);
    }
  }
 

  return true;
}
/*
   void run_shell(int fd_in = 0,int fd_out = 1 ,int fd_err = 2){
    // init
    init_pipe_pool();
    setenv("PATH","bin:.",1);
    // set all stdin/stdout/stderr to socket
    fd_reopen(FD_STDIN,fd_in);
    fd_reopen(FD_STDOUT,fd_out);
    fd_reopen(FD_STDERR,fd_err);
    cout << "****************************************" << endl
    << "** Welcome to the information server. **" << endl
    << "****************************************" << endl;
  string str;
  unblock_sig_msg();
  while(true){
    cout << "% ";
    if(!getline(cin,str)){
#ifdef DEBUG
      debug("stream closed");
#endif
      break;
    }
    str = string_strip(str);
    smatch match;
    if(regex_search(str, regex("^\\s*exit(\\s+|$)"))){
      break;
    }
    else if(regex_search(str, regex("/"))){
      cout << "/ is forbidden here!" << endl;
    }
    // who
    else if(regex_search(str, regex("^\\s*who(\\s+|$)"))){
      cout << "<ID>\t<nickname>\t<IP/port>\t<indicate me>" << endl;
      conn_lock();
      for(ConnClientEntry& ent : client_p->p_mgmt->clients){
        if(ent.is_valid()){
          cout << ent.id << '\t' << ent.nick << '\t' << ent.ip << '/' << ent.port << '\t';
          if(&ent == client_p){
            // me
            cout << "<-me";
          }
          cout << endl;
        }
      }
      conn_unlock();
    }
    // change name
    else if(regex_search(str,match,regex("^\\s*name(\\s+|$)"))){
      string new_name = string_strip(match.suffix());
      if(!new_name.empty()){
        conn_lock();
        if(client_p->change_name(new_name)){
          // already broadcasted 
        }
        else{
          cout << "*** User '" << new_name <<"' already exists. ***" << endl;
        }
        conn_unlock();
      }
      else{
        cout << "*** Cannot use an empty name ***" << endl;
      }
    }
    // messaging
    // tell
    else if(regex_search(str,match,regex("^\\s*tell\\s+(\\d+)(\\s|$)"))){
      int tell_id = std::stoi(match[1]);
      string msg = string_strip(match.suffix());
      if(!msg.empty()){
        conn_lock();
        if(!client_p->send_tell_message(tell_id,msg)){
          // failure
          cout << "*** Error: user #" << match[1] <<" does not exist yet. ***" << endl;
        }
        conn_unlock();
      }
    }
    // yell, not yell to self
    else if(regex_search(str,match,regex("^\\s*yell(\\s|$)"))){
      string msg = string_strip(match.suffix());
      if(!msg.empty()){
        conn_lock();
        client_p->send_yell_message(msg);
        conn_unlock();
      }
    }

    // Env related
    else if(regex_search(str,match,regex("^\\s*printenv(\\s+|$)"))){
      string name = string_strip(match.suffix());
      if(!name.empty()){
        cout << name << '=' << getenv(name.c_str()) << endl;
      }
    }
    else if(regex_search(str,match,regex("^\\s*setenv(\\s+|$)"))){
      string remain = match.suffix();
      if(regex_search(remain,match,regex("([^ \t\n]*)\\s+([^ \t\n]*)"))){
        string var = match[1];
        string name = match[2];
        setenv(var.c_str(),name.c_str(),1);
      }
    }
    else{
      UnixPipe next_pipe(false); 
      UnixPipe last_pipe(false); 
      bool first = true;
      bool last = false;
      UnixPipe stdout_pipe(false);
      int stdout_pipe_count = -1;
      UnixPipe stderr_pipe(false);
      int stderr_pipe_count = -1;
      string cut_string = str;
      string next_cut;
      while(regex_search(cut_string,match,regex("(.+?)($|(\\| )|(([|!]\\d+\\s*)+))"))){
        next_cut = match.suffix();
        string src_cmd_line = string_strip(match[1]);
        string cmd_line;
        string suffix = string_strip(match[2]);
        UnixPipe error_pipe;
       
        // pipe pool
        if(first){
          if(!pipe_pool.empty()){
            pipe_pool.pop_front();
            if(!pipe_pool.empty()){
              pipe_pool[0].close_write();
            }
          }
        }
        int pub_write_pipe = -1;
        int pub_read_pipe = -1;
        cmd_line = src_cmd_line;
        // read pub pipe
        if(regex_search(cmd_line,match,regex("<([^!| \t\n]+)"))){
          // for public pipe
          pub_read_pipe = std::stoi(match[1]);
          cmd_line = string_strip(match.prefix()) + " " + string(match.suffix());
        }

        // Writing File name and write pub pipe
        string filename;
        const char* filemode = "";
        if(regex_search(cmd_line,match,regex("(>+)(\\s*)([^!| \t\n]+)"))){
          string sep = match[2];
          if(sep.empty()){
            // for public pipe
            pub_write_pipe = std::stoi(match[3]);
          }
          else{
            string mode_str = string_strip(match[1]);
            if(mode_str == ">>"){
              filemode = "a";
            }
            else{
              filemode = "w";
            }
            filename = string_strip(match[3]);
          }
          cmd_line = string_strip(match.prefix()) + " " + string(match.suffix());
        }
        // arguments
        string::size_type space_pos = cmd_line.find_first_of(" \t");
        string cmd;
        string arg_line;
        if(space_pos != string::npos){
          cmd = cmd_line.substr(0,space_pos);
          arg_line = string(cmd_line.substr(space_pos));   
        }
        else{
          cmd = cmd_line; 
        }
#ifdef DEBUG
        debug("cmd:"+cmd);
        debug("arg:"+arg_line);
         
        stringstream ss1;
        ss1 << "public write pipe:" << pub_write_pipe;
        debug(ss1.str());


        stringstream ss2;
        ss2 << "public read pipe:" << pub_read_pipe;
        debug(ss2.str());

        debug("filename:"+filename);
        debug("filemode:"+string(filemode));
        debug("suffix:"+suffix);
#endif
        if(suffix=="|"){
          last = false;
          next_pipe = UnixPipe();
        }
        else{
          last = true; 
          if(suffix.size() >= 2){
            // create numbered-pipe
            // stdout
            if(regex_search(suffix,match,regex("\\|(\\d+)"))){
              stdout_pipe_count = stoi(match[1]);
              stdout_pipe = create_numbered_pipe(stdout_pipe_count);
            }
            // stderr
            if(regex_search(suffix,match,regex("!(\\d+)"))){
              stderr_pipe_count = stoi(match[1]);
              stderr_pipe = create_numbered_pipe(stderr_pipe_count);
            }
            
          }
        }
        // fork
        pid_t pid;
        if((pid = fork())>0){
          // parent
          // close last_pipe and change it to next_pipe
          last_pipe.close();
          if(!last){
            last_pipe = next_pipe;
          }
          // read from error_pipe, if 0 readed. child is successfully exited
          error_pipe.close_write();
          string result = string_strip(error_pipe.getline());
          if(!result.empty()){
            // error
            if(result=="unknown"){
              printf("Unknown command: [%s].\n",src_cmd_line.c_str());
            }
            else if(result=="pipe existed"){ // write pipe existed
              cout << "*** Error: public pipe #" << pub_write_pipe <<" already exists. ***" << endl;
            }
            else if(result=="pipe not exist"){ // read pipe not existed
              cout << "*** Error: public pipe #" << pub_read_pipe <<" does not exist yet. *** " << endl;
            }
            // restore pipe
            if(first){
              pipe_pool.push_front(UnixPipe(false));
            }
            // close any numbered-pipe if exist
            stdout_pipe.close(); 
            stdout_pipe.close();
            break;
          }
          else{
            // success
            if(first){
              first = false; 
              if(!pipe_pool.empty()){
                pipe_pool[0].close();
              }
            }
            record_pipe_info(stdout_pipe_count,stdout_pipe);
            record_pipe_info(stderr_pipe_count,stderr_pipe);
            if(pub_write_pipe >= 0){
              stringstream ss;
              ss << "*** " << client_p->nick <<" (#" << client_p->id <<") just piped '" << str << "' ***\n";
              conn_lock();
              p_mgmt->send_broadcast_message(ss.str());
              conn_unlock();
            }
            if(pub_read_pipe >= 0){
              stringstream ss;
              ss << "*** " << client_p->nick <<" (#" << client_p->id <<") just received via '" << str << "' ***\n";
              conn_lock();
              p_mgmt->send_broadcast_message(ss.str());
              conn_unlock();
            }
          }
          // close error pipe
          error_pipe.close_read();
          // wait for child
          waitpid(pid,nullptr,0);
          // close an used public pipe
          if(pub_read_pipe >= 0){
            pipe_lock();
            PubPipe::p_mgmt->clear_pipe(pub_read_pipe);
            pipe_unlock();
          }
        }else{
          // child
          // close last pipe writing
          last_pipe.close_write();
          // close error pipe read
          error_pipe.close_read();
          // stdout redirect
          if(!filename.empty()){
            // file
            FILE* f = fopen(filename.c_str(), filemode);
            fd_reopen(FD_STDOUT,fileno(f));
          }
          else{
            if(last){
              if(!stdout_pipe.is_write_closed()){
                // open with numbered-pipe 
                fd_reopen(FD_STDOUT,stdout_pipe.write_fd);
              }
              else if (pub_write_pipe >= 0){
                // try to create to public pipe
                pipe_lock();
                int fd = PubPipe::p_mgmt->open_write(pub_write_pipe);
                pipe_unlock();
                if(fd >= 0){
                  // public write pipe success
                  fd_reopen(FD_STDOUT,fd);
                }
                else{
                  exit_error(error_pipe,"pipe existed");
                }

              }
            }
            else{
              // open with next pipe 
              fd_reopen(FD_STDOUT,next_pipe.write_fd);
            }
          }
          // stderr redirect: numbered-pipe
          if(!stderr_pipe.is_write_closed()){
            // open with numbered-pipe 
            fd_reopen(FD_STDERR,stderr_pipe.write_fd);
          }
          // first process open numbered pipe
          if(first){
            if(pub_read_pipe >= 0){
              pipe_lock();
              int fd = PubPipe::p_mgmt->open_read(pub_read_pipe);
              pipe_unlock();
              if(fd >= 0){
                fd_reopen(FD_STDIN,fd);
              }
              else{
                exit_error(error_pipe,"pipe not exist");
              }
              close_pipe_larger_than(0);
            }
            else{
              // the first process, try to open numbered pipe
              UnixPipe& n_pipe = pipe_pool[0];
              if(!n_pipe.is_read_closed()){
                n_pipe.close_write();
                fd_reopen(FD_STDIN,n_pipe.read_fd); 
              }
              // close other high-count pipe
              close_pipe_larger_than(1);

            }
          }
          else{
            // non-first, open last pipe
            fd_reopen(FD_STDIN,last_pipe.read_fd); 
            // close all numbered-pipe
            close_pipe_larger_than(0);
          }
          // Exec
          exec_cmd(cmd,arg_line);
          // if you are here, means error occurs
          exit_error(error_pipe,"unknown");
        }
        // prepare for next cut
        cut_string = std::move(next_cut);
      } // end for line-cutting
    } // end for normal cmd
  } // end for while 

}
*/
#endif
