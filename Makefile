all: smallchat

smallchat: 
	g++ main.cc chatserver.cc epoll.cc -o smallchat -O2 -g -Wall -W -std=c++11

clean:
	rm -f smallchat