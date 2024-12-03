#include "headerlist.h"

#ifndef HTTPDEFINE
#define HTTPDEFINE

#define HTTP_BUF_MAX_SIZE 1024

#endif

#ifndef HTTPSTRUCT
#define HTTPSTRUCT

struct HTTPRequest
{
    char method[8];
    char path[256];
    int content_length;
    char body[HTTP_BUF_MAX_SIZE];
};

#endif

//functions 
void parse_http_request(const char *request, struct HTTPRequest *http_request);
void send_file_content(int cli, const char *file_path);
void send_quiz(int cli);