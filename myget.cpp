#include"./myget.h"



const char * get_mime_type(const char * name)
{
    //自右向左查找‘.’字符;如不存在返回NULL
    const char * dot = strrchr(name, '.');
    /*
     *charset=iso-8859-1	西欧的编码, 说明网站采用的编码是英文；
     *charset=gb2312		说明网站采用的编码是简体中文；
     *charset=utf-8			代表世界通用的语言编码；
     *						可以用到中文、韩文、日文等世界上所有语言编码上
     *charset=euc-kr		说明网站采用的编码是韩文；
     *charset=big5			说明网站采用的编码是繁体中文；
     *
     *以下是依据传递进来的文件名, 使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
     */
    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void strdecode(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from) {

        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {

            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else
            *to = *from;
    }
    *to = '\0';
}

int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}



void HttpRequest(int rwsock,int epfd)
{
    char buf[1024];
    char buffer[1024];
    char meth[16];
    char url[256];
    char protocal[16];
    int n;
    memset(&buf,'\0',sizeof(buf));
    n = get_line(rwsock,buf,sizeof(buf));
    if(n<=0)
    {
        //发送出现异常关闭连接
        close(rwsock);
        epoll_ctl(epfd,EPOLL_CTL_DEL,rwsock,nullptr);
        return;
    }
    cout << buf <<endl;
    sscanf(buf,"%[^ ] %[^ ] %[^ /r/n]",meth,url,protocal);
    cout << meth << endl;
    cout << url << endl;
    cout << protocal << endl;

    
    char* pfile = url;
    if(strlen(url)<=1)
    {
        strcpy(pfile,"./");
    }else
    {
        pfile = url+1;
    }

    //转换汉字编码
    strdecode(pfile,pfile);
    cout << pfile << endl;

    while((n=get_line(rwsock,buf,sizeof(buf)))>0);
    struct stat st;
    
    if(stat(pfile,&st)<0)
    {
        cout << "file not exit" <<endl;
        send_header(rwsock,"404","NOT Found",get_mime_type(".html"),0);
        send_file(rwsock,"error.html");
    }else
    {
        if(S_ISREG(st.st_mode))
        {
            send_header(rwsock,"200","OK",get_mime_type(pfile),st.st_size);
            send_file(rwsock,pfile);
        }else
        {
            //发现是dir文件
            send_header(rwsock,"200","OK",get_mime_type(".html"),0);
            //发送目录的头
            send_file(rwsock,"./html/dir_header.html");

            //获取dir文件内的所有文件名,发送目录的中间
            struct dirent **namelist;
			int num;
            num = scandir(pfile, &namelist, NULL, alphasort);
            if (num < 0)
			{
			   perror("scandir error");
			   close(rwsock);
			   epoll_ctl(epfd, EPOLL_CTL_DEL, rwsock, NULL);
			   return ;
			}
            else
            {
                while(num--)
                {
                    cout << namelist[num]->d_name << endl;
                    memset(buffer,'\0',sizeof(buffer));
                    if(namelist[num]->d_type == DT_DIR)
                    {
                        sprintf(buffer,"<li><a href=%s/>%s</a></li>",namelist[num]->d_name,namelist[num]->d_name);
                    }else
                    {
                        sprintf(buffer,"<li><a href=%s>%s</a></li>",namelist[num]->d_name,namelist[num]->d_name);
                    }
                    write(rwsock,buffer,sizeof(buffer));
                    free(namelist[num]);
                }
                free(namelist);
            }

            //发送目录的尾部
            send_file(rwsock,"./html/dir_tail.html");
        }
    }

}



void send_header(int rwsock,const char* code,const char* msg,const char* filetype,int len)
{
    char buf[1024];
    memset(&buf,'\0',sizeof(buf));
    sprintf(buf,"HTTP/1.1 %s %s\r\n",code,msg);
    sprintf(buf+strlen(buf),"Content-Type:%s\r\n",filetype);
    if(len>0)
    {
        sprintf(buf+strlen(buf),"Content-Length:%d\r\n",len);
    }
    strcat(buf,"\r\n");
    int n = write(rwsock,buf,sizeof(buf));
    if(n<=0)
    {
        perror("write error");
    }
}

int send_file(int rwsock,const char* filename)
{
    int fd = open(filename, O_RDONLY);
    char buf[1024];
    if(fd<0)
    {
        perror("open error");
        return -1;
    }
    while(1)
    {
        memset(&buf,'\0',sizeof(buf));
        int n = read(fd,buf,sizeof(buf));
        if(n<=0)
        {
            perror("read error");
            break;
        }
        write(rwsock,buf,n);
    }
}


int get_line(int sock, char *buf,int size){
  int count =0;
  char ch = '\0';
  int len = 0;

  while((count<size-1)&&ch!='\n'){
    len = read(sock,&ch,1);
    if(len==1){
      if(ch=='\r'){
        continue;
      }else if(ch=='\n'){
        buf[count] = '\0';
        break;
      }

      buf[count] = ch;
      count++;
    }else if(len==-1){
      perror("read failed");
      count = -1;
      break;
    }else{
      cerr << "client close.\n" <<endl;
      count = -1;
      break;
    }
  }
  return count;
}




void* getconnect()
{
    while(1)
    {
        sleep(sleeptime);
        int count = 0;
        unique_lock<mutex> lock(mtx);
        for(auto i = mp.begin(); i != mp.end();i++)
        {
            if(i->second.tm!=-1)
            {
                count++;
            }
        }
        cout << "当前服务器连接的数量:" << count << endl;
    }
}

void* checkconnect(void* arg)
{
    int epfd = *(int*)arg;
    time_t nowtime,cnttime;
    int timeout;
    struct sockaddr_in cntaddr;
    char ip[16];
    while(1)
    {
        sleep(sleeptime);
        unique_lock<mutex> lock(mtx);
        for(auto i = mp.begin();i != mp.end();i++)
        {
            if(i->second.tm != -1)
            {
                nowtime = time(nullptr);
                cnttime = i->second.tm;
                timeout = difftime(nowtime,cnttime);
                cntaddr = i->second.addr;
                if(timeout > Maxtimeout)
                {
                    i->second.tm = -1;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,i->first,nullptr);
                    cout << "连接" << i->first << "超时" << endl;
                    cout << "ip地址是" << inet_ntop(AF_INET,&cntaddr.sin_addr.s_addr,ip,sizeof(ip)) << endl;
                    memset(&ip,'\0',sizeof(ip));
                    close(i->first);
                }
                else
                {
                    //未超时的处理
                }
            }
        }
    }
}