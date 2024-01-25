#ifndef UTILS_H
#define UTILS_H

#define _GNU_SOURCE

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/file.h>
#include <ctype.h>


#define PORT 8023
#define IP "127.0.0.1"

#define OP_LIST 0x0ul
#define OP_DOWNLOAD 0x1ul
#define OP_UPLOAD 0x2ul
#define OP_DELETE 0x4ul
#define OP_MOVE 0x8ul
#define OP_UPDATE 0x10ul
#define OP_SEARCH 0x20ul

#define S_SUCCES 0x0ul
#define S_FILE_NOT_FOUND 0x1ul
#define S_PERM_DENIED 0x2ul
#define S_OUT_OF_MEM 0x4ul
#define S_SERVER_BUSY 0x8ul
#define S_UNKNOWN_OP 0x10ul
#define S_BAD_ARGS 0x20ul
#define S_UNDEFINED_ERR 0x40ul

#define MAX_BUFFER 1024

/*
        Functiile de trimitere si repceptionare sunt inspirate de aici:
        https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c

        Functiile lsrec sunt inspirate de aici:
        https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux

*/

int send_data(int sockfd, const void *buffer, size_t bufsize);
int receive_data(int sockfd, void *buffer, size_t bufsize);
int send_uint32(int sockfd, uint32_t data);
int receive_uint32(int sockfd, uint32_t *data);
char* extractDirPath(const char *filePath);
void _mkdir(const char *dir);
int createFileWithDirectories(const char *filePath);
int receive_data_to_file(int socket, int fd, size_t file_size);
void lsrec_getsize(char *dir, uint32_t *size);
void lsrec_setbuff_helper(const char *dir, char *buffer, size_t *offset);
void lsrec_setbuff(const char *dir, char *buffer);

#endif // UTILS_H
