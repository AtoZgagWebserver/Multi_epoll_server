#include "headerlist.h"
#include "httpfunc.h"

#ifndef EPOLLSTRUCT
#define EPOLLSTRUCT

#define MAX_EVENT_SIZE 4096

struct EpollManager
{
    int ep;
    pthread_t tid;
};

#endif

struct EpollManager* make_epoll_manager(int numb);
void add_fd_to_manager(int ep, int fd);
void del_fd_from_manager(int ep,int fd);