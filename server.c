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

// CORS 헤더 전송 함수
void send_cors_headers(int client_fd) {
    const char *cors_header =
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n" // 모든 출처 허용, 특정 도메인으로 변경 가능 (예: 'http://localhost:3000')
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Type: application/json\r\n\r\n";

    // CORS 헤더를 먼저 전송
    send(client_fd, cors_header, strlen(cors_header), 0);
}

// 클라이언트의 요청을 처리하는 함수
void handle_client_request(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // 종료 문자 추가

        // 요청이 OPTIONS 메서드인 경우
        if (strncmp(buffer, "OPTIONS", 7) == 0) {
            send_cors_headers(client_fd);
        } else {
            // GET/POST 요청 처리
            send_cors_headers(client_fd); // 기본 응답
            // 추가적인 요청 처리 로직은 여기에 추가 가능합니다.
        }
    }
    close(client_fd);
}

int main(int argc, char* argv[])
{
    // 기본 설정
    if(argc != 3) { // ./server portnum managernum workernum
        perror("Argument num is wrong");
        exit(1);
    }
    int portnum = atoi(argv[1]), mannum = atoi(argv[2]);
    struct sockaddr_in sin, cli;
    int sd, clientlen = sizeof(cli), cnt = 0;

    // 소켓 생성
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
    {
        perror("socket");
        exit(1);
    }

    // 포트 재사용 설정
    int optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 소켓 구조체 설정
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(portnum);
    sin.sin_addr.s_addr = inet_addr("0.0.0.0");

    // 바인딩
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)))
    {
        perror("bind error");
        exit(1);
    }
    if(listen(sd, 50000)) // 백로그 크기
    {
        perror("listen error");
        exit(1);
    }

    set_nonblocking(sd);

    // 클라이언트 준비
    printf("Prepare to accept client\n");

    // epoll 관리자 설정
    printf("    Create epoll manager.\n");
    struct EpollManager* epm_list = make_epoll_manager(mannum); 
    printf("    Create epoll manager done.\n");
    
    printf("    Read data\n");
    question = read_gag();
    printf("    Read gag done\n");

    printf("    Prepare server epoll man\n");
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

    // 클라이언트 수락 및 처리
    int ns, ev_num;

    while(1)
    {
        ev_num = epoll_wait(sv_ep, events, MAX_EVENT, -1);
        if(ev_num == -1)
        {
            perror("epoll_wait sv_ep");
            exit(1);
        }
        for (int i = 0; i < ev_num; i++) {
            if (events[i].data.fd == sd) {
                // 새 클라이언트 수락
                ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
            if (ns == -1) {
                perror("accept");
                continue;
            }
            set_nonblocking(ns);
            //send_cors_headers(ns);
            add_fd_to_manager(epm_list[cnt].ep, ns);
            cnt = (cnt + 1) % mannum;
            } 
        }
    }
    close(sd);
    return 0;
}
