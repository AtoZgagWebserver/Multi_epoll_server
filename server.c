#include "lib/headerlist.h"
#include "lib/readdata.h"
#include "lib/processepoll.h"

#define MAX_EVENT 1024

struct QuestionList *question;

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[])
{
    //default set
    if(argc != 3)//./server portnum managernum workernum
    {
        perror("Argument num is wrong");
        exit(1);
    }
    int portnum = atoi(argv[1]), mannum = atoi(argv[2]);
    struct sockaddr_in sin, cli;
    int sd, clientlen = sizeof(cli), cnt = 0;
    
    // create socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
    {
        perror("socket");
        exit(1);
    }

    // port recycle
    int optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // create socket struct
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(portnum);
    sin.sin_addr.s_addr = inet_addr("0.0.0.0");

    // bind
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)))
    {
        perror("bind error");
        exit(1);
    }
    if(listen(sd, 50000)) // second parameter is BACKLOG That is max size of queue
    {
        perror("listen error");
        exit(1);
    }

    set_nonblocking(sd);

    // prepare for clients
    printf("Prepare to accept client\n");

    printf("    Create epoll manager.\n");
    struct EpollManager* epm_list = make_epoll_manager(mannum); 
    printf("    Create epoll manager done.\n");
    
    printf("    Read data\n");
    question = read_gag();
    printf("    Read gag done\n");

    printf("    Preapre server epoll man\n");
    int sv_ep = epoll_create1(0);
    if(sv_ep == -1)
    {
        perror("sv_ep");
        exit(1);
    }
    struct epoll_event ev = {0}, events[MAX_EVENT];
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    if(epoll_ctl(sv_ep,EPOLL_CTL_ADD, sd, &ev) == -1)
    {
        perror("epoll_ctl serv");
        exit(1);
    }
    printf("    Prepare server epoll man done.\n");

    printf("Ready for accept client\n");

    // accept the clients
    int ns,ev_num;
    while(1)
    {
        ev_num = epoll_wait(sv_ep, events, MAX_EVENT, -1);
        if(ev_num == -1)
        {
            perror("epoll_wait sv_ep");
            exit(1);
        }
		if((ns=accept(sd,(struct sockaddr*)&cli,&clientlen))==-1)
        {
			perror("accept");
			exit(1);
		}
        set_nonblocking(ns);
        add_fd_to_manager(epm_list[cnt].ep, ns);
        cnt=(cnt+1)%mannum;
    }
	close(sd);
}