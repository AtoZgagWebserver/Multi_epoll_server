#include "httpfunc.h"
#include "readdata.h"

extern struct QuestionList *question;

void parse_http_request(const char *request, struct HTTPRequest *http_request) {
    //요청 메소드, 경로, 내용 길이, 내용 만 일단 추출
    char *pos;

    // 요청 메소드 (GET, POST) 추출
    if (strncmp(request, "GET", 3) == 0) 
    {
        strcpy(http_request->method, "GET");
        pos = strchr(request + 4, ' ');
        if (pos != NULL) 
        {
            strncpy(http_request->path, request + 4, pos - (request + 4));
            http_request->path[pos - (request + 4)] = '\0';
        }
    } 
    else if (strncmp(request, "POST", 4) == 0)
    {
        strcpy(http_request->method, "POST");
        pos = strchr(request + 5, ' ');
        if (pos != NULL) 
        {
            strncpy(http_request->path, request + 5, pos - (request + 5));
            http_request->path[pos - (request + 5)] = '\0';
        }
    } 
    else 
    {
        strcpy(http_request->method, "UNKNOWN");
    }

    http_request->content_length = 0;
    pos = strstr(request, "Content-Length: ");
    if (pos != NULL) 
    {
        http_request->content_length = atoi(pos + 16);
    }

    pos = strstr(request, "\r\n\r\n");
    if (pos != NULL) 
    {
        strcpy(http_request->body, pos + 4);
    } 
    else 
    {
        http_request->body[0] = '\0';
    }
}

void send_file_content(int cli, const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) 
    {
        printf("file not found\n");
        const char *not_found_response = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";
        send(cli, not_found_response, strlen(not_found_response), 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = malloc(file_size + 1);
    if (file_content == NULL) 
    {
        perror("malloc");
        fclose(file);
        return;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';

    char header[256];
    snprintf(header, sizeof(header), 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n" //html로 테스트, css,javascript,csv, 이미지, 오디오, 비디오 등 다양한 형태가 있어서
             "Content-Length: %ld\r\n" //요청한 데이터가 어떤건지 확인한느 부분이 필요?
             "\r\n", file_size);
    send(cli, header, strlen(header), 0);
    send(cli, file_content, file_size, 0);

    fclose(file);
}

void send_quiz(int cli){
    int count=10;
    char buf[1024 * 10] = "[\n"; // JSON 시작
    char temp[1024];
    struct Question* q;
    q = get_random_Question(question,count);

    for (int i = 0; i < count; i++) {
        // 각 질문과 답을 JSON 형식으로 temp에 작성
        sprintf(temp, "  {\n");
        strcat(buf, temp);

        sprintf(temp, "    \"question\": \"%s\",\n", q[i].quest);
        strcat(buf, temp);

        sprintf(temp, "    \"answer\": \"%s\"\n", q[i].ans);
        strcat(buf, temp);

        if (i < count - 1) {
            sprintf(temp, "  },\n");
        } else {
            sprintf(temp, "  }\n");
        }
        strcat(buf, temp);
    }

    strcat(buf, "]"); // JSON 종료

    char header[256];
    snprintf(header, sizeof(header), 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n" //html로 테스트, css,javascript,csv, 이미지, 오디오, 비디오 등 다양한 형태가 있어서
             "Content-Length: %ld\r\n" //요청한 데이터가 어떤건지 확인한느 부분이 필요?
             "\r\n", strlen(buf));
    send(cli, header, strlen(header), 0);
    send(cli,buf,strlen(buf),0);
}