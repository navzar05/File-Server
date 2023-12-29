#include "server.h"

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

void gracious_exit(pthread_t maintid) {
        pthread_cancel(maintid);
}

void *thread_quit(void *args) {
    struct thread_quit_args *arguments = (struct thread_quit_args *)args;
    sigset_t mask;

    // Set the signal mask for this thread
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    int epollfd = epoll_create1(0);
    struct epoll_event ev;
    struct epoll_event retev;
    int sfd;

    sfd = signalfd(-1, &mask, 0);

    // Add standard input for checking if 'quit' is written
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    // Add signal file descriptor
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev);

    while (1) {
        epoll_wait(epollfd, &retev, 2, -1);
        if ((retev.events & EPOLLIN) != 0) {
            if (retev.data.fd == sfd) {
                struct signalfd_siginfo si;
                ssize_t s = read(sfd, &si, sizeof(si));
                if (s != sizeof(si)) {
                    perror("read");
                    break;
                }

                // Handle signals
                if (si.ssi_signo == SIGINT || si.ssi_signo == SIGTERM) {
                    printf("Hello from signal, exiting...\n");
                    gracious_exit(arguments->maintid);
                    break;
                }
            } else if (retev.data.fd == STDIN_FILENO) {
                char input[50];
                fgets(input, sizeof(input), stdin);
                size_t len = strlen(input);
                if (len > 0 && input[len - 1] == '\n') {
                    input[len - 1] = '\0';
                }
                if (strcmp(input, "quit") == 0) {
                    printf("Received 'quit', exiting...\n");
                    gracious_exit(arguments->maintid);
                    break;
                }
            }
        }
    }

    free(arguments);

    return NULL;
}

int log_operation(const char* operation, const char* filename, const char* word_searched)
{
        pthread_mutex_lock(&mutex_log);

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        FILE* fp = fopen(LOG_FILE_PATH, "a");

        if (fp == 0) {
                fprintf(stderr, "Error on opening logging file.\n");
                return -1;
        }

        // printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        if (strncmp(operation, "LIST", 4) == 0)
                fprintf(fp, "%d-%02d-%02d, %02d:%02d, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation);
        else if (strncmp(operation, "SEARCH", 6) == 0)
                fprintf(fp, "%d-%02d-%02d, %02d:%02d, %s, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation, word_searched);
        else
                fprintf(fp, "%d-%02d-%02d, %02d:%02d, %s, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation, filename);
        fclose(fp);

        pthread_mutex_unlock(&mutex_log);
        return 0;
}

int server_init(int* listenSock)
{
        struct sockaddr_in address;

	memset(&address, 0, sizeof(struct sockaddr_in));
        address.sin_addr.s_addr = inet_addr(IP);
        address.sin_port = htons(PORT);
        address.sin_family = AF_INET;

        (*listenSock) = socket(AF_INET, SOCK_STREAM, 0);

        if ((*listenSock) < 0) {
                perror("Error on creating listening socket");
                return -1;
        }

        if (bind((*listenSock), (struct sockaddr*)(&address), sizeof(address)) < 0) {
                perror("Error on binding");
                return -1;
        }


	 if ( listen((*listenSock), 1) < 0) {
                perror("Error on listening");
                return -1;
         }

        sigset_t mask;

        // creez un file descriptor pentru semnalele SIGTERM si CTRL + C (SIGINT)
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        // le blochez pe thread-ul principal
        sigprocmask(SIG_BLOCK, &mask, NULL);

        struct thread_quit_args *arguments = (struct thread_quit_args *)calloc(1, sizeof(struct thread_quit_args));

        arguments->mask = mask;
        arguments->maintid = pthread_self();

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

        // cream thread-ul pentru a asculta pentru incheierea executiei
        pthread_t tid_quit;

        int ret = pthread_create(&tid_quit, NULL, thread_quit, arguments);
        if (ret < 0) {
                fprintf(stderr, "Thread failed to be created.\n");
        }

        // pthread_detach(tid);

        return 0;
}

int server_cleanup()
{
        return 0;
}

void lsrec_getsize(char *dir, uint32_t *size)
{
   char path[MAX_PATH];
   struct dirent *dp;
   DIR *dfd;

   if ((dfd = opendir(dir)) == NULL) {
      fprintf(stderr, "lsrec: can't open %s\n", dir);
      return;
   }

   while ((dp = readdir(dfd)) != NULL) {
        if (dp->d_type == 4){ // #define DT_DIR 4
                path[0] = '\0';
                if (strcmp(dp->d_name, ".") == 0 ||
                strcmp(dp->d_name, "..") == 0)
                continue;
                sprintf(path, "%s/%s", dir, dp->d_name);
                lsrec_getsize(path, size);
        }
        else{
                // printf("%s/%s size = %d\n", dir, dp->d_name, strlen(dir) + strlen(dp->d_name) + 2);
                (*size) += strlen(dir) + strlen(dp->d_name) + 2;
        }
      }
   closedir(dfd);
}

void lsrec_setbuff(char *dir, char *buffer, uint32_t *offset)
{
    char path[MAX_PATH];
    struct dirent *dp;
    DIR *dfd;

    if ((dfd = opendir(dir)) == NULL)
    {
        fprintf(stderr, "lsrec: can't open %s\n", dir);
        return;
    }

    while ((dp = readdir(dfd)) != NULL)
    {
        if (dp->d_type == 4)
        {
            path[0] = '\0';
            if (strcmp(dp->d_name, ".") == 0 ||
                strcmp(dp->d_name, "..") == 0)
                continue;
            sprintf(path, "%s/%s", dir, dp->d_name);
            lsrec_setbuff(path, buffer, offset);
        }
        else
        {
            // printf("%s/%s size = %d\n", dir, dp->d_name, strlen(dir) + strlen(dp->d_name) + 2);
            strncat(buffer + (*offset), dir, MAX_PATH - 1);
            strcat(buffer + (*offset), "/");
            strncat(buffer + (*offset), dp->d_name, MAX_PATH - 1 - strlen(dir) - 2);
            strcat(buffer + (*offset), "\0");
            (*offset) += strlen(buffer + (*offset)) + 1;
        }
    }
    closedir(dfd);
}

void generateRealPath(const char *relativePath, char *realPath)
{
    // Assuming that "./files/" is the prefix for the files
    if(relativePath[0] == '.' && relativePath[1] == '/') {
        //printf("%s\n", relativePath + 2);
        snprintf(realPath, strlen(relativePath) + sizeof("./files/") - 2, "./files/%s", relativePath + 2);
    }
    else {
        snprintf(realPath, strlen(relativePath) + sizeof("./files/"), "./files/%s", relativePath);
    }
}

int list(int socket)
{
        struct dirent *de = NULL;
        uint32_t size_to_send = 0;
        uint32_t offset = 0;
        DIR *dr = opendir(SERVER_DIR);
        char* buffer = NULL;

        lsrec_getsize("./files", &size_to_send);

        //printf("size_to_send: %d\n", size_to_send);

        buffer = (char*)calloc(size_to_send, sizeof(char));

        lsrec_setbuff("./files", buffer, &offset);

        if (dr == NULL) {
                printf("Could not open current directory" );
                return -1;
        }

        const char *current = buffer;

/*         while (*current != '\0') {
                printf("String: %s\n", current);
                current += strlen(current) + 1;
        } */

        // send status
        send_uint32(socket, S_SUCCES);

        // send size of data
        send_uint32(socket, size_to_send);

        // send data
        send_data(socket, buffer, size_to_send);

        log_operation("LIST", NULL, NULL);

        closedir(dr);

        free(buffer);

        return 0;
}

int download(int socket)
{

        uint32_t size = 0;
        uint32_t size_to_send = 0;
        char* path = NULL;
        char* real_path = NULL;

        if (receive_uint32(socket, &size) <= 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        printf("size: %d\n", size);

        path = (char*)calloc(size, sizeof(char));
        real_path = (char*)calloc(size + sizeof("./files/"), sizeof(char));

        if (path == NULL) {
                send_uint32(socket, S_OUT_OF_MEM);
                return -1;
        }

        receive_data(socket, path, size);

        printf("path: %s\n", path);

        generateRealPath(path, real_path);

        int fd = open(real_path, O_RDONLY);

        printf("fd: %d\n", fd);

        if (fd < 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                perror("open");
                return -1;
        }

        send_uint32(socket, S_SUCCES);

        struct stat buf;
        fstat(fd, &buf);
        size_to_send = buf.st_size;

        printf("size_to_send: %d\n", size_to_send);

        send_uint32(socket, size_to_send);

        sendfile(socket, fd, NULL, size_to_send);

        log_operation("DOWNLOAD", path, NULL);

        close(fd);

        free(path);

        free(real_path);

        return 0;
}

int upload(int socket)
{
        uint32_t path_size = 0;
        char* path = NULL;
        char* real_path = NULL;
        uint32_t file_size = 0;

        if (receive_uint32(socket, &path_size) <= 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        printf("path size: %d\n", path_size);

        path = (char*)calloc(path_size, sizeof(char));

        real_path = (char*)calloc(path_size + sizeof("./files/"), sizeof(char));

        if (receive_data(socket, path, path_size) <= 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        generateRealPath(path, real_path);

        printf("%s\n", real_path);

        createFileWithDirectories(real_path);

        int fd = open(real_path, O_RDWR);

        if (fd < 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        receive_uint32(socket, &file_size);

        printf("file size = %d\n", file_size);

        receive_data_to_file(socket, fd, file_size);

        send_uint32(socket, S_SUCCES);

        log_operation("UPLOAD", path, NULL);

        free(path);

        free(real_path);

        close(fd);

        return 0;
}

int delete(int socket)
{
        int path_size = 0;
        char* path = NULL;
        char* real_path = NULL;

        if (receive_uint32(socket, &path_size) <= 0){
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        path = (char*)calloc(path_size, sizeof(char));

        real_path = (char*)calloc(path_size + sizeof("./files/"), sizeof(char));

        if (path == NULL) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        receive_data(socket, path, path_size);

        generateRealPath(path, real_path);

        if (remove(real_path) < 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        send_uint32(socket, S_SUCCES);

        log_operation("DELETE", path, NULL);

        free(path);

        free(real_path);

        return 0;
}

int move(int socket)
{
        uint32_t size_src = -1;
        uint32_t size_dest = -1;
        char *src = NULL;
        char *dest = NULL;
        char* real_path_src = NULL;
        char* real_path_dest = NULL;

        if (receive_uint32(socket, &size_src) <= 0) {
                // send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        src = (char*)calloc(size_src, sizeof(char));

        real_path_src = (char*)calloc(size_src + sizeof("./files/"), sizeof(char));

        receive_data(socket, src, size_src);

        generateRealPath(src, real_path_src);

        if (receive_uint32(socket, &size_dest) <= 0) {
                // send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        dest = (char*)calloc(size_dest, sizeof(char));

        real_path_dest = (char*)calloc(size_dest + sizeof("./files/"), sizeof(char));

        receive_data(socket, dest, size_dest);

        generateRealPath(dest, real_path_dest);

        if (access(real_path_src, F_OK) != 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        if (access(real_path_src, R_OK | W_OK) != 0) {
                send_uint32(socket, S_PERM_DENINED);
                return -1;
        }

        rename(real_path_src, real_path_dest);

        send_uint32(socket, S_SUCCES);

        log_operation("MOVE", src, NULL);


        free(src);
        free(dest);
        free(real_path_dest);
        free(real_path_src);

        return 0;

}

int update(int socket)
{

        // "data" sunt datele care vor urma sa fie adaugate in fisier

        int fd = 0; // file descriptor pentru fisier care va fi modificat
        uint32_t size_path = -1;
        uint32_t size_data = -1;
        uint32_t offset = -1;
        char* path = NULL;
        char* real_path = NULL;
        char* data = NULL;

        receive_uint32(socket, &size_path);

        path = (char*)calloc(size_path, sizeof(char));

        real_path = (char*)calloc(size_path + sizeof("./files/"), sizeof(char));

        data = (char*)calloc(size_data, sizeof(char));

        receive_data(socket, path, size_path);

        generateRealPath(path, real_path);

        receive_uint32(socket, &offset);

        receive_uint32(socket, &size_data);

        receive_data(socket, data, size_data);

        if (access(real_path, F_OK) != 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        if (access(real_path, W_OK) != 0) {
                send_uint32(socket, S_PERM_DENINED);
                return -1;
        }

        if ((fd = open(real_path, O_RDWR)) < 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }


        lseek(fd, offset, SEEK_SET);

        write(fd, data, size_data);

        send_uint32(socket, S_SUCCES);

        log_operation("UPDATE", path, NULL);

        free(path);

        free(real_path);

        free(data);

        return 0;
}