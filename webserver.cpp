#include"./myget.h"



mutex mtx;
unordered_map<int,info> mp;

int main()
{
    //若web服务器给浏览器发送数据的时候, 浏览器已经关闭连接, 
	//则web服务器就会收到SIGPIPE信号
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGPIPE, &act, NULL);


    //改变当前进程的工作目录
	char path[255] = {'\0'};
    getcwd(path, sizeof(path));
	strcat(path, "/webpath");
	chdir(path);
    cout << path << endl;

    int rwsock;
    int ready;
    int sock = socket(AF_INET,SOCK_STREAM,0);
    char buf[256];

    int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr,'\0',sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(22000);
    bind(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));

    listen(sock,128);
    cout << "服务器正在监听"  << endl;

    int epfd = epoll_create(256);
    if(epfd < 0)
    {
        perror("epoll_create error:");
        return -1;
    }
    struct epoll_event ev;
    struct epoll_event event[256];
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sock,&ev);

    mp.clear();

    thread check(checkconnect,&epfd);
    thread count(getconnect);



    while(1)
    {
        ready = epoll_wait(epfd,event,256,-1);
        if(ready<0)
        {
            perror("epoll_wait error");
            if(errno = EINTR)
            {
                continue;
            }else
            {
                break;
            }
        }
        unique_lock<mutex> lock(mtx);
        for(int i=0;i<ready;i++)
        {
            rwsock = event[i].data.fd;
            if(rwsock==sock)
            {
                struct sockaddr_in address;
                socklen_t len = sizeof(address);
                int cfd = accept(sock,(struct sockaddr*)&address,&len);

                int flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);

                ev.data.fd = cfd;
                ev.events = EPOLLIN;//设置epoll的边缘触发模式 | EPOLLET
                epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
                
                mp[cfd].tm = time(nullptr);
                mp[cfd].addr = address;
            }
            else
            {
                mp[rwsock].tm = time(nullptr);
                HttpRequest(rwsock,epfd);
            }
        }
    }
    check.join();
    count.join();
    close(epfd);
    close(sock);
    return 0; 
}



