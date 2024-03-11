#ifndef _MYGET_H_
#define _MYGET_H_

#include<string.h>
#include<iostream>
#include<thread>
#include<unistd.h>
#include<algorithm>
#include<ctype.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include<mutex>

using namespace std;



const int sleeptime = 30;
const int Maxtimeout = 30;




struct info 
{
    time_t tm;
    struct sockaddr_in addr;
};

extern mutex mtx;
extern unordered_map<int,info> mp;


const char * get_mime_type(const char * name);
void strdecode(char *to, char *from);
int hexit(char c);
void HttpRequest(int rwsock,int epfd);
void send_header(int rwsock,const char* code,const char* msg,const char* filetype,int len);
int send_file(int rwsock,const char* filename);
int get_line(int sock, char *buf,int size);


void* getconnect();
void* checkconnect(void* arg);


#endif