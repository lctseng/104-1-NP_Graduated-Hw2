all: server-v1 server-v2
server-v1: v1/conn_mgmt.hpp v1/fdstream.hpp v1/lib.hpp v1/pipe.hpp v1/shell.hpp v1/server.cpp 
	@echo using `g++ --version | grep ^g++ `
	@GCC_MAJOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$1}'` ;\
	GCC_MINOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$2}'` ;\
	GCCVERSION=`expr $$GCC_MAJOR \* 10000 + $$GCC_MINOR ` ;\
	if [ $$GCCVERSION -ge 50000 ];\
	then \
	  echo "compiling as executable: server-v1" ;\
	  g++ --std=c++11 v1/server.cpp -o server-v1;\
	else \
	  rm -f server ;\
	  echo "Please use g++ 5.0.0+ version" ;\
	fi
server-v2: v2/conn_mgmt.hpp v2/fdstream.hpp v2/lib.hpp v2/lock.hpp v2/pipe.hpp v2/pub_pipe_mgmt.hpp v2/semaphore.hpp v2/shell.hpp v2/server.cpp 
	@echo using `g++ --version | grep ^g++ `
	@GCC_MAJOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$1}'` ;\
	GCC_MINOR=`g++ --version | grep ^g++ | sed 's/^.* //g' | awk -F. '{print $$2}'` ;\
	GCCVERSION=`expr $$GCC_MAJOR \* 10000 + $$GCC_MINOR ` ;\
	if [ $$GCCVERSION -ge 50000 ];\
	then \
	  echo "compiling as executable: server-v2" ;\
	  g++ --std=c++11 v2/server.cpp -o server-v2;\
	else \
	  rm -f server ;\
	  echo "Please use g++ 5.0.0+ version" ;\
	fi
clean:
	rm -f server-v1
	rm -f server-v2
