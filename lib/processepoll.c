#include "processepoll.h"

int handle_client(int ep, char *msg, int ns)
{
    struct HTTPRequest http_request = {0};
    parse_http_request(msg, &http_request);

    if (strcmp(http_request.method, "GET") == 0)
    {
        if (strcmp(http_request.path, "/quiz") == 0)
        {
            send_quiz(ns);
        }
        else if (strcmp(http_request.path, "/score") == 0)
        {
            get_score(ns);
        }
        else
        {
            char file_path[512];
            snprintf(
                file_path,
                sizeof(file_path),
                "./rsc/html/%s",
                http_request.path[0] == '/' ? http_request.path + 1 : http_request.path);
            send_file_content(ns, file_path);
        }
    }
    else if (strcmp(http_request.method, "POST") == 0)
    {
        if (strcmp(http_request.path, "/score") == 0)
        {
            post_score(ns, http_request.body);
        }
    }
    else
    {
        const char *not_found_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n404 Not Found";
        send(ns, not_found_response, strlen(not_found_response), 0);
    }

    close(ns);
    del_fd_from_manager(ep, ns);

    return 0;
}

void *epoll_manager(void *arg)
{
    struct EpollManager *epm = (struct EpollManager *)arg;
    struct epoll_event ev[MAX_EVENT_SIZE];
    char buf[4096];
    int ev_size, tmp, size;
    while (1)
    {
        ev_size = epoll_wait(epm->ep, ev, MAX_EVENT_SIZE, -1); // 이벤트 발생까지 대기
        if (ev_size < 0)
        {
            perror("epoll_wait Error");
            exit(1);
        }
        for (int i = 0; i < ev_size; i++)
        {
            int total = 0;
            while(1)
            {
                size = read(ev[i].data.fd, buf-total, sizeof(buf) - 1 - total);
                if(size <=0)
                    break;
                total+=size;
            }
            if (total <= 0)
            {
                close(ev[i].data.fd);
                del_fd_from_manager(epm->ep, ev[i].data.fd);
                continue;
            }
            handle_client(epm->ep, buf, ev[i].data.fd);
        }
    }
}

struct EpollManager *make_epoll_manager(int numb)
{
    struct EpollManager *epm_list = (struct EpollManager *)calloc(sizeof(struct EpollManager), numb);
    for (int i = 0; i < numb; i++)
    {
        epm_list[i].ep = epoll_create1(0);
        pthread_create(&epm_list[i].tid, NULL, epoll_manager, (void *)&epm_list[i]);
    }

    return epm_list;
}

void add_fd_to_manager(int ep, int fd)
{
    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);
}

void del_fd_from_manager(int ep, int fd)
{
    epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL);
}