#!/usr/bin/env ruby 

require 'fileutils'
require 'socket'

PORT=ARGV[0].to_i
MODE=:normal#:console
#$DEBUG=true

def debug(*args)
  #$stderr.print ":::DEBUG:::"
  #$stderr.puts *args
end

def wait_for_children(children)
  children.each do |pid|
    begin
      Process.wait pid
    rescue Errno::ECHILD
    end
  end
  children.clear
end

def exec_cmd(cmd,argv)
  begin
    Process.exec(cmd,*argv)
  rescue Errno::ENOENT => e
    return nil
  end
end

def exit_unknown_cmd(cmd,error_pipe)
  error_pipe[1].write "error"
  error_pipe[1].close
  abort
end

def create_numbered_pipe(count)
  $pipe_pool[count] || IO.pipe
end

def record_pipe_info(info)
  if info
    count = info[:count]
    $pipe_pool[count] = info[:pipe]
  end
end
def close_pipe_info(info)
  if info
    info[:pipe].each {|p| p.close}
  end
end

def close_pipe_larger_than(count)
  for i in (count+1)...$pipe_pool.size
    pipe = $pipe_pool[i]
    if pipe
      pipe.each {|p| p.close}
    end
  end
end

def decrease_pipe_counter_and_close
  $pipe_pool.shift
  out_pipe = $pipe_pool[0]
  if out_pipe
    out_pipe.each {|p| p.close}
  end
end

def handle_client(client_sock)
  # initialize
  $pipe_pool = []
  $legal_counter = 0
  FileUtils.cd 'ras'
  ENV["PATH"] = "bin:."
  
  # reopen sock
  if client_sock
    $stdout.reopen client_sock
    $stderr.reopen client_sock
    $stdin.reopen client_sock
  end
  
  # run
  puts <<DOC
****************************************
** Welcome to the information server. **
****************************************
DOC
  children = []
  while true
    print '% '
    line = $stdin.gets
    if line =~ /^\s*exit/
      debug "Bye!"
      break
    elsif line =~ /^\s*printenv(\s*|$)/
      $' =~ /(.*)/
      if !$1.empty?
        var = $1.strip
        puts "#{var}=#{ENV[var]}"
      end
      decrease_pipe_counter_and_close
    elsif line =~ /^\s*setenv(\s*|$)/
      $' =~ /([^ \t\n]*)\s+([^ \t\n]*)/
      if $1 && !$1.empty?
        ENV[$1.strip] = $2.strip
      end
      decrease_pipe_counter_and_close
    else
      next_pipe = nil
      last_pipe = nil
      first = true
      stdout_pipe_info = nil
      stderr_pipe_info = nil
      process_cnt = 0
      # parse each cmd
      while line =~ /(.+?)($|(\| )|([|!]\d+\s*)+)/i
        if first
          $pipe_pool.shift
        end
        line =  $'.strip
        error_pipe = IO.pipe
        cmd_line = $1.strip
        suffix = $2.strip
        filename = nil
        if cmd_line =~ />\s*([^!| \t\n]+)/
          filename = $1
          cmd_line = "#{$`} #{$'}"
        end
        debug "FILE:#{filename}"
        debug "CMD line:#{cmd_line}"
        argv = cmd_line.split(' ')
        cmd =  argv.shift
        if suffix == '|'
          debug "normal: #{cmd}"
          # open a normal pipe
          next_pipe = IO.pipe
          last = false
        else
          debug "last  : #{cmd},suffix:#{suffix}"
          last = true
          if suffix.size > 0
            # create numbered-pipe
            # stdout pipe
            if suffix =~ /\|(\d+)/i
              count = $1.to_i
              stdout_pipe_info = {
                count: count,
                pipe: create_numbered_pipe(count)
              }
              debug "stdout pipe after #{count} commands"
            end
            # stderr pipe
            if suffix =~ /!(\d+)/i
              count = $1.to_i
              stderr_pipe_info = {
                count: count,
                pipe: create_numbered_pipe(count)
              }
              debug "stderr pipe after #{count} commands"
            end
          end

        end
        if pid = fork
          # parent
          children << pid
          # close last_pipe and change it to next_pipe
          last_pipe.each {|p| p.close} if last_pipe
          last_pipe = next_pipe if !last
          # read from error_pipe, if 0 readed. child is successfully exited
          error_pipe[1].close
          result = error_pipe[0].read(10)
          if result
            puts "Unknown command: [#{cmd}]."
            if first
              $pipe_pool.unshift nil
            end
            # close any pipe info if exist
            close_pipe_info(stdout_pipe_info)
            close_pipe_info(stderr_pipe_info)
            debug "child #{cmd} is error! Abort!!"
            break
          else
            debug "child #{cmd} is ok!"
            process_cnt += 1
            debug "Process #:#{process_cnt}"
            if first 
              $legal_counter += 1
              first = false
              n_pipe = $pipe_pool[0]
              if n_pipe
                n_pipe.each {|p| p.close}
              end
              debug "legal counter:#{$legal_counter}"
            end
            record_pipe_info(stdout_pipe_info)
            record_pipe_info(stderr_pipe_info)
          end
          error_pipe[0].close
          wait_for_children(children)
        else
          # child
          # close error pipe read 
          error_pipe[0].close
          # stdout redirect 
          if filename
            # to a file
            $stdout.reopen File.open(filename,'w')
          else
            # open stdout with next_pipe - write
            if !last
              $stdout.reopen next_pipe[1]
            end
            # numbered-pipe
            if stdout_pipe_info
              $stdout.reopen stdout_pipe_info[:pipe][1]
            end
          end
          if stderr_pipe_info
            $stderr.reopen stderr_pipe_info[:pipe][1]
          end
          # first process open numbered pipe
          if first
            # the first process, try to open numbered pipe
            n_pipe = $pipe_pool[0]
            if n_pipe
              n_pipe[1].close
              $stdin.reopen n_pipe[0]
            end
            close_pipe_larger_than(1)
          else
            # open stdin with last_pipe - read
            $stdin.reopen last_pipe[0]
            # close all numbered pipe
            close_pipe_larger_than(0)
          end
          # Exec
          exec_cmd(cmd,argv)
          # if you are here, means error occurs
          exit_unknown_cmd(cmd,error_pipe)
        end # end fork
      end
    end
  end
end

Signal.trap("CHLD") do
  begin
    while pid = Process.wait(-1,Process::WNOHANG)
    end
  rescue Errno::ECHILD

  end
end

# Main
if MODE == :console
  handle_client(nil)
else
  listen_sock = TCPServer.new('0.0.0.0',PORT)
  while client_sock = listen_sock.accept
    if pid = fork
      # parent
      client_sock.close
    else
      # child
      listen_sock.close
      handle_client(client_sock)
      exit
    end
  end
  listen_sock.close
end
