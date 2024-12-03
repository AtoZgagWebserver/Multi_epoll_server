#include "httpfunc.h"
#include "readdata.h"

extern struct QuestionList *question;

void parse_http_request(const char *request, struct HTTPRequest *http_request)
{
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

void send_file_content(int cli, const char *file_path)
{
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
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             file_size);
    send(cli, header, strlen(header), 0);
    send(cli, file_content, file_size, 0);

    fclose(file);
}

void send_quiz(int cli)
{
    int count = 10;
    char buf[1024 * 10] = "[\n"; // JSON 시작
    char temp[1024];
    struct Question *q;
    q = get_random_Question(question, count);

    for (int i = 0; i < count; i++)
    {
        // 각 질문과 답을 JSON 형식으로 temp에 작성
        sprintf(temp, "  {\n");
        strcat(buf, temp);

        sprintf(temp, "    \"question\": \"%s\",\n", q[i].quest);
        strcat(buf, temp);

        sprintf(temp, "    \"answer\": \"%s\"\n", q[i].ans);
        strcat(buf, temp);

        if (i < count - 1)
        {
            sprintf(temp, "  },\n");
        }
        else
        {
            sprintf(temp, "  }\n");
        }
        strcat(buf, temp);
    }

    strcat(buf, "]"); // JSON 종료

    send(cli, buf, strlen(buf), 0);
}

void get_score(int cli)
{
    char buf[1024 * 10] = ""; // 결과 JSON 데이터를 담을 버퍼
    FILE *file = fopen("./rsc/score.txt", "r");

    if (!file)
    {
        char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n"
                                "Content-Type: text/plain\r\n"
                                "\r\n"
                                "Failed to open score file.";
        send(cli, error_response, strlen(error_response), 0);
        return;
    }

    // JSON 데이터 읽기
    fread(buf, 1, sizeof(buf), file);
    fclose(file);

    send(cli, buf, strlen(buf), 0);
}

void post_score(int cli, const char *body)
{
    char nickname[256] = "";
    int score = 0;

    // 요청 본문에서 JSON 데이터를 파싱
    if (sscanf(body, "{\"nickname\":\"%255[^\"]\",\"score\":%d}", nickname, &score) != 2)
    {
        char error_response[] = "HTTP/1.1 400 Bad Request\r\n"
                                "Content-Type: text/plain\r\n"
                                "\r\n"
                                "Invalid JSON format.";
        send(cli, error_response, strlen(error_response), 0);
        return;
    }

    // 기존 점수 파일 읽기
    FILE *file = fopen("./rsc/score.txt", "r");
    if (!file)
    {
        char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n"
                                "Content-Type: text/plain\r\n"
                                "\r\n"
                                "Failed to open score file.";
        send(cli, error_response, strlen(error_response), 0);
        return;
    }

    // 점수 목록을 배열에 저장
    char existing_scores[1024 * 10];
    int count = 0;
    while (fgets(existing_scores + count, sizeof(existing_scores) - count, file))
    {
        count = strlen(existing_scores); // 기존 점수 내용의 끝까지 읽기
    }
    fclose(file);

    // 기존 점수들을 파싱해서 배열에 저장 (순위 및 점수 파싱)
    char nicknames[100][256];  // 최대 100개의 점수 저장
    int scores[100];
    int num_scores = 0;

    char *line = strtok(existing_scores, "\n");
    while (line != NULL)
    {
        if (sscanf(line, "  {\"nickname\":\"%255[^\"]\",\"score\":%d}", nicknames[num_scores], &scores[num_scores]) == 2)
        {
            num_scores++;
        }
        line = strtok(NULL, "\n");
    }

    // 새로운 점수 추가
    strcpy(nicknames[num_scores], nickname);
    scores[num_scores] = score;
    num_scores++;

    // 점수 내림차순 정렬 (버블 정렬)
    for (int i = 0; i < num_scores - 1; i++)
    {
        for (int j = i + 1; j < num_scores; j++)
        {
            if (scores[i] < scores[j])
            {
                // 점수 교환
                int temp_score = scores[i];
                scores[i] = scores[j];
                scores[j] = temp_score;

                // 닉네임 교환
                char temp_name[256];
                strcpy(temp_name, nicknames[i]);
                strcpy(nicknames[i], nicknames[j]);
                strcpy(nicknames[j], temp_name);
            }
        }
    }

    // 점수 파일에 다시 저장
    file = fopen("./rsc/score.txt", "w");
    if (!file)
    {
        char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n"
                                "Content-Type: text/plain\r\n"
                                "\r\n"
                                "Failed to write score file.";
        send(cli, error_response, strlen(error_response), 0);
        return;
    }

    // JSON 형식으로 저장
    fprintf(file, "[\n");
    for (int i = 0; i < num_scores; i++)
    {
        fprintf(file, "  {\"nickname\":\"%s\",\"score\":%d}\n", nicknames[i], scores[i]);
        if (i < num_scores - 1)
            fprintf(file, ",\n");
    }
    fprintf(file, "]\n");
    fclose(file);

    // 성공 응답 반환
    char success_response[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "\r\n"
                              "Score saved and sorted successfully.";
    send(cli, success_response, strlen(success_response), 0);
}