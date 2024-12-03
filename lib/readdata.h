#ifndef STRUCT_DATA
#define STRUCT_DATA

//structs
struct Question
{
    char* quest;
    char* ans;
};

struct QuestionList
{
    struct Question* item;
    int size;
    int maxsize;
};

#endif

//functions
struct QuestionList* read_gag();
struct Question* get_random_Question(struct QuestionList* q_list, int q_size);