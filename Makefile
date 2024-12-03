CC = gcc
CFLAGS = -g

TARGET = server

SRCS = server.c lib/processepoll.c lib/httpfunc.c lib/readdata.c 
OBJS = $(SRCS:.c=.o)

all: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c #$<는 현재 의존 파일 $@는 타겟을 의미함.
	$(CC) $(CFLAGS) -c $< -o $@

clear:
	rm -f $(OBJS) $(TARGET)
