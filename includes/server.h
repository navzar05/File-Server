// #include "utils/net_utils.h"
#ifndef SERVER_H
#define SERVER_H

#define _XOPEN_SOURCE 500
#include "utils.h"
#include <dirent.h>
#include <string.h>
#include <ftw.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/file.h>

#define PORT 8017
#define IP "127.0.0.1"

#define MAX_PATH 1024
#define MAX_NO_THREADS 10
#define SERVER_DIR "/home/razvan/Desktop/TEMA2/files"
#define LOG_FILE_PATH "/home/razvan/Desktop/TEMA2/logger.log"

struct thread_quit_args {
        pthread_t maintid;
        sigset_t mask;
};

void gracious_exit(pthread_t maintid);

void *thread_quit(void *args);

int log_operation(const char* operation, const char* filename, const char* word_searched);

int server_init(int* listenSock);
int server_cleanup();

void lsrec_getsize(char *dir, uint32_t *size);
void lsrec_setbuff(char *dir, char *buffer, uint32_t *offset);

void clear_socket_buffer(int socket);
int list(int socket);
int download(int socket);
int upload(int socket);
int delete(int socket);
int move(int socket);
int update(int socket);

#endif // SERVER_H
