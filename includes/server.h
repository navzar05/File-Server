// #include "utils/net_utils.h"
#ifndef SERVER_H
#define SERVER_H

#include "utils.h"

#define INCREASE_RATIO 1.5
#define DEFAULT_NO_WORDS 10
#define MAX_WORDS 10
#define MAX_WORD_LENGTH 100

#define MAX_NO_THREADS 10
#define ROOT_DIR "./files"
#define LOG_FILE_PATH "./logger.log"

struct thread_quit_args {
        pthread_t maintid;
        sigset_t mask;
};

struct word_freq_map {
    uint32_t frequency;
    char word[MAX_WORD_LENGTH];
};

struct freq_list {
    char *filename;
    struct word_freq_map *word_map;
};

void gracious_exit(pthread_t maintid);

void *thread_quit(void *args);

void *thread_index(void *args);

int log_operation(const char* operation, const char* filename, const char* word_searched);

int server_init(int* listenSock);
int server_cleanup();
int update_file_list();

int list(int socket);
int download(int socket);
int upload(int socket);
int delete(int socket);
int move(int socket);
int update(int socket);
int search(int socket);

#endif // SERVER_H
