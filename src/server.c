#include "server.h"

// mutex pentru log
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

// tid pentru thread-ul de indexare
pthread_t tid_index = -1;

size_t index_of_threads = 0;
pthread_t tids[MAX_NO_THREADS] = { 0 };
pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex pentru structura globala in care tin freq_list pentru fiecare fisier
pthread_mutex_t mutex_freq_list = PTHREAD_MUTEX_INITIALIZER;
struct freq_list *global_freq_list;
uint32_t global_freq_list_size = 0;

// mutex si variabila conditionala pentru thread-ul de indexare
pthread_mutex_t mutex_cont_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var_cont_exec = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_file_list = PTHREAD_MUTEX_INITIALIZER;

char *file_list = NULL;
uint32_t size_file_list = 0; // dimensiunea maxima

int listenSock = 0;

void gracious_exit(pthread_t maintid)
{

        while (index_of_threads > 0) {
                // printf("waiting on %d job...\n", tids[index_of_threads - 1]);
                pthread_join(tids[index_of_threads - 1], NULL);
        }

        printf("All jobs finished...\n");

        if(close(listenSock) < 0) {
                perror("failed to close listen sock");
        } else {
                printf("Listen socket closed...\n");
        }
        if (pthread_cancel(tid_index) < 0) {
                perror("failed to cancel index thread");
        } else {
                printf("Index thread canceled...\n");
        }
        if (pthread_cancel(maintid) < 0) {
                perror("failed to cancel main thread");
        } else {
                printf("Main thread canceled...\n");
        }

        if (global_freq_list_size != 0) {
                        int i = 0;
                        while (i < global_freq_list_size) {
                                free(global_freq_list[i].word_map);
                                global_freq_list[i].word_map = NULL;
                                free(global_freq_list[i].filename);
                                global_freq_list[i].filename = NULL;
                                i++;
                        }
                        free(global_freq_list);
        }

        if (file_list != NULL)
                free(file_list);

        pthread_cond_destroy(&cond_var_cont_exec);

}

void *thread_quit(void *args)
{
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
        epoll_wait(epollfd, &retev, 1, -1);
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

int isSeparator(const char ch) {
    return (ch == '\n' || ch == '\t' || ch == ',' || ch == '.' ||
            ch == '?' || ch == '!' || ch == '@' || ch == '(' ||
            ch == ')' || ch == '\\' || ch == ';');
}

int read_file(const int fd, char *buffer, const size_t size)
{
        ssize_t bytesRead;
        size_t totalBytesRead = 0;

        while (totalBytesRead < size) {
                bytesRead = read(fd, buffer + totalBytesRead, size - totalBytesRead);

                if (bytesRead == 0) {
                        break;
                }
                if (bytesRead == -1) {
                if (errno == EINTR)
                        continue;
                return -1;
                }

                totalBytesRead += bytesRead;
        }

        return 0;
}

int process_token(size_t *size, struct word_freq_map **map, const char *token) {
        int i = 0;

        // printf("size: %d\n", (*size));

        while (i < (*size) && (token != NULL)) {

                if (strlen((*map)[i].word) == 0) {
                        strncpy((*map)[i].word, token, strlen(token));
                        (*map)[i].frequency = 1;
                        return 0;
                }

                if (strncmp((*map)[i].word, token, MAX_WORD_LENGTH) == 0) {
                        (*map)[i].frequency++;
                        return 0;
                }
                i++;
        }

        if (i == (*size)) {

                // Realocam memorie daca e nevoie
                struct word_freq_map *temp_map = realloc(*map, (*size) * INCREASE_RATIO * sizeof(struct word_freq_map));

                if (temp_map == NULL) {
                        perror("Memory reallocation failed");
                        return -1;
                }

                *map = temp_map;
                (*size) = (*size) * INCREASE_RATIO;

                // printf("new size: %d\n", (*size));


        }

        strncpy((*map)[i].word, token, strlen(token));

        (*map)[i].frequency = 1;


        return 0;
}

int compare_freq_map(const void *a, const void *b) {
    const struct word_freq_map *map_a = (const struct word_freq_map *)a;
    const struct word_freq_map *map_b = (const struct word_freq_map *)b;

    return map_b->frequency - map_a->frequency;
}

int get_freq_list(const char* filename, const int index)
{
	int fd = open(filename, O_RDONLY);

        char* token = NULL;
        char* file_buffer = NULL;
        char* saveptr = NULL;
        const char delims[] = " .\\,!?$^@#;:\n\t\0";

        // lista cuvinte temporara pentru fisier
        struct word_freq_map* file_word_map = (struct word_freq_map*)calloc(DEFAULT_NO_WORDS, sizeof(struct word_freq_map));
        size_t size_file_word_map = DEFAULT_NO_WORDS;
        // global_freq_list[index] = (struct freq_list*)calloc(1, sizeof(struct freq_list));
	// lista celor 10 cuvinte pentru fisier
        global_freq_list[index].word_map = (struct word_freq_map*)calloc(DEFAULT_NO_WORDS, sizeof(struct word_freq_map));
        size_t size_of_file = lseek(fd, 0, SEEK_END);

        // printf("size: %d\n", size_of_file);

        lseek(fd, 0, SEEK_SET);

        file_buffer = (char*)calloc(size_of_file, sizeof(char));

        flock(fd, LOCK_SH);

        // printf("LOCK AQUIRED\n");

        read_file(fd, file_buffer, size_of_file);

        global_freq_list[index].filename = (char*)calloc(strlen(filename) + 1, sizeof(char));

	memcpy(global_freq_list[index].filename, filename, strlen(filename));

        token = strtok_r(file_buffer, delims, &saveptr);

        while(token != NULL) {

                process_token(&size_file_word_map, &file_word_map, token);

                token = strtok_r(NULL, delims, &saveptr);
        }

        qsort(file_word_map, size_file_word_map, sizeof(struct word_freq_map), compare_freq_map);

        file_word_map = (struct word_freq_map*)realloc(file_word_map, DEFAULT_NO_WORDS * sizeof(struct word_freq_map));

        memcpy(global_freq_list[index].word_map, file_word_map, DEFAULT_NO_WORDS * sizeof(struct word_freq_map));

	flock(fd, LOCK_UN);

        free(file_word_map);

        free(file_buffer);

        close(fd);

	return 0;
}

void *thread_index(void *args)
{
        pthread_cond_init(&cond_var_cont_exec, NULL);

        while (1) {

                // uint32_t size_of_paths = size_file_list;

                char* current_path = NULL;
                // char* current_data = NULL;

                pthread_mutex_lock(&mutex_freq_list);

                pthread_mutex_lock(&mutex_file_list);

                if (global_freq_list_size != 0) {
                        int i = 0;
                        while (i < global_freq_list_size) {
                                free(global_freq_list[i].word_map);
                                global_freq_list[i].word_map = NULL;
                                free(global_freq_list[i].filename);
                                global_freq_list[i].filename = NULL;
                                i++;
                        }
                        free(global_freq_list);
                }

                global_freq_list_size = 0;

                /* lsrec_getsize(ROOT_DIR, &size_of_paths);

                paths = (char*)calloc(size_of_paths, sizeof(char));

                lsrec_setbuff(ROOT_DIR, paths); */

                //calculam numarul de intrari in global_freq_map (egal cu numarul de fisiere)

                current_path = file_list;

                while (*current_path != '\0') {
                        global_freq_list_size++;
                        current_path += strlen(current_path) + 1;
                }

                // printf("%d\n", global_freq_list_size);
                //alocam memoria necesara

                global_freq_list = (struct freq_list*)calloc(global_freq_list_size, sizeof(struct freq_list));

                if (global_freq_list == NULL) {
                        printf("%s","EROORRR\n");
                }

                //procesam fiecare fisier si adaugam intrarea corespunzatoare

                current_path = file_list;

                int i = 0;

               // printf("global: %d\n", global_freq_list_size);

                while ( current_path != NULL && i < global_freq_list_size) {
                        // printf("Proc file %s\n", current_path);
                        get_freq_list(current_path, i);
                        i++;
                        current_path += strlen(current_path) + 1;
                }

                pthread_mutex_unlock(&mutex_file_list);

                pthread_mutex_unlock(&mutex_freq_list);

                pthread_mutex_lock(&mutex_cont_exec);

                pthread_cond_wait(&cond_var_cont_exec, &mutex_cont_exec);

                pthread_mutex_unlock(&mutex_cont_exec);



        }

        pthread_cond_destroy(&cond_var_cont_exec);

	return NULL;
}

int log_operation(const char* operation, const char* filename, const char* word_searched)
{
    int fd = open(LOG_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1) {
        perror("Error opening logging file");
        return -1;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Error acquiring file lock");
        close(fd);
        return -1;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if (strncmp(operation, "LIST", 4) == 0)
        dprintf(fd, "%d-%02d-%02d, %02d:%02d, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation);
    else if (strncmp(operation, "SEARCH", 6) == 0)
        dprintf(fd, "%d-%02d-%02d, %02d:%02d, %s, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation, word_searched);
    else
        dprintf(fd, "%d-%02d-%02d, %02d:%02d, %s, %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,tm.tm_hour, tm.tm_min, operation, filename);

    if (flock(fd, LOCK_UN) == -1) {
        perror("Error releasing file lock");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int server_init(int* listenSock)
{
        struct sockaddr_in address;

	memset(&address, 0, sizeof(struct sockaddr_in));
        address.sin_addr.s_addr = inet_addr(IP);
        address.sin_port = htons(PORT);
        address.sin_family = AF_INET;
        int reuse = 1;

        (*listenSock) = socket(AF_INET, SOCK_STREAM, 0);

        if ((*listenSock) < 0) {
                perror("Error on creating listening socket");
                return -1;
        }

        if (setsockopt((*listenSock), SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
                perror("setsockopt(SO_REUSEADDR) failed");
                return -1;
        }

        if (bind((*listenSock), (struct sockaddr*)(&address), sizeof(address)) < 0) {
                perror("Error on binding");
                return -1;
        }


	 if ( listen((*listenSock), MAX_NO_THREADS) < 0) {
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

        pthread_detach(tid_quit);

        update_file_list();

	pthread_create(&tid_index, NULL, thread_index, NULL);

        pthread_detach(tid_quit);

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

void lsrec_setbuff_helper(const char *dir, char *buffer, size_t *offset) {
    char path[MAX_PATH];
    struct dirent *dp;
    DIR *dfd;

    if ((dfd = opendir(dir)) == NULL) {
        fprintf(stderr, "lsrec: can't open %s\n", dir);
        return;
    }

    while ((dp = readdir(dfd)) != NULL) {
        if (dp->d_type == 4) {
            path[0] = '\0';
            if (strcmp(dp->d_name, ".") == 0 ||
                strcmp(dp->d_name, "..") == 0)
                continue;
            sprintf(path, "%s/%s", dir, dp->d_name);
            lsrec_setbuff_helper(path, buffer, offset);
        } else {
            strncat(buffer + (*offset), dir, MAX_PATH - 1);
            strcat(buffer + (*offset), "/");
            strncat(buffer + (*offset), dp->d_name, MAX_PATH - 1 - strlen(dir) - 2);
            strcat(buffer + (*offset), "\0");
            (*offset) += strlen(buffer + (*offset)) + 1;
        }
    }
    closedir(dfd);
}

void lsrec_setbuff(const char *dir, char *buffer) {
    size_t offset = 0;
    lsrec_setbuff_helper(dir, buffer, &offset);
}

void generateRealPath(const char *relativePath, char *realPath)
{
    if(relativePath[0] == '.' && relativePath[1] == '/') {
        snprintf(realPath, strlen(relativePath) + sizeof("./files/") - 2, "./files/%s", relativePath + 2);
    }
    else {
        snprintf(realPath, strlen(relativePath) + sizeof("./files/"), "./files/%s", relativePath);
    }
}

int update_file_list()
{
        pthread_mutex_lock(&mutex_file_list);

        lsrec_getsize(ROOT_DIR, &size_file_list);

        printf("list size %d\n", size_file_list);

        if (file_list != NULL) {
                free(file_list);
                file_list = NULL;
        }



        file_list = (char*)calloc(size_file_list, sizeof(char));

        lsrec_setbuff(ROOT_DIR, file_list);

        pthread_mutex_unlock(&mutex_file_list);
        return 0;
}

int list(int socket)
{
        DIR *dr = opendir(ROOT_DIR);



        if (dr == NULL) {
                printf("Could not open current directory" );
                return -1;
        }

        if (file_list == NULL) {
                update_file_list();
        }

        pthread_mutex_lock(&mutex_file_list);

        // send status
        send_uint32(socket, S_SUCCES);

        // send size of data
        send_uint32(socket, size_file_list);

        // send data
        send_data(socket, file_list, size_file_list);

        pthread_mutex_unlock(&mutex_file_list);

        log_operation("LIST", NULL, NULL);

        closedir(dr);

        return 0;
}

int download(int socket)
{

        uint32_t size = 0;
        uint32_t size_to_send = 0;
        char* path = NULL;
        char* real_path = NULL;
        int fd = -1;

        receive_uint32(socket, &size);

        if (size != -1)
        {
                path = (char*)calloc(size, sizeof(char));
                real_path = (char*)calloc(size + sizeof("./files/"), sizeof(char));

                receive_data(socket, path, size);

                // printf("path: %s\n", path);

                generateRealPath(path, real_path);

                fd = open(real_path, O_RDONLY);
        }

        if (fd == -1 && size == -1) {
                send_uint32(socket, S_BAD_ARGS);
                return -1;
        }

        if (fd < 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                perror("open");
                return -1;
        }

        flock(fd, LOCK_SH);

        send_uint32(socket, S_SUCCES);

        struct stat buf;
        fstat(fd, &buf);
        size_to_send = buf.st_size;

        send_uint32(socket, size_to_send);

        sendfile(socket, fd, NULL, size_to_send);

        log_operation("DOWNLOAD", path, NULL);

        flock(fd, LOCK_UN);

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

        receive_uint32(socket, &path_size);

        if (path_size == -1) {
                send_uint32(socket, S_BAD_ARGS);
                return -1;
        }

        path = (char*)calloc(path_size, sizeof(char));

        real_path = (char*)calloc(path_size + sizeof("./files/"), sizeof(char));

        receive_data(socket, path, path_size);

        generateRealPath(path, real_path);


        int fd = createFileWithDirectories(real_path);

        flock(fd, LOCK_SH);

        receive_uint32(socket, &file_size);

        receive_data_to_file(socket, fd, file_size);

        if (access(real_path, F_OK) > 0) {
                send_uint32(socket, S_BAD_ARGS);
                goto ERR_BAD_ARGS;
        }

        else if (fd < 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                goto ERR_FD;
        }

        else
                send_uint32(socket, S_SUCCES);

        update_file_list();

        pthread_cond_signal(&cond_var_cont_exec);

        log_operation("UPLOAD", path, NULL);


        ERR_BAD_ARGS:

        close(fd);

        ERR_FD:
        flock(fd, LOCK_UN);

        free(path);

        free(real_path);

        return 0;
}

int delete(int socket)
{
        uint32_t path_size = 0;
        char* path = NULL;
        char* real_path = NULL;

        receive_uint32(socket, &path_size);

        if (path_size == -1) {
                send_uint32(socket, S_BAD_ARGS);
                return -1;
        }

        path = (char*)calloc(path_size, sizeof(char));

        real_path = (char*)calloc(path_size + sizeof("./files/"), sizeof(char));

        receive_data(socket, path, path_size);

        generateRealPath(path, real_path);

        if (remove(real_path) < 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        send_uint32(socket, S_SUCCES);

        update_file_list();

        log_operation("DELETE", path, NULL);

        free(path);

        free(real_path);

        pthread_cond_signal(&cond_var_cont_exec);

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

        receive_uint32(socket, &size_src);

        if (size_src == -1) {
                send_uint32(socket, S_BAD_ARGS);
                return -1;
        }

        src = (char*)calloc(size_src, sizeof(char));

        real_path_src = (char*)calloc(size_src + sizeof("./files/"), sizeof(char));

        receive_data(socket, src, size_src);

        generateRealPath(src, real_path_src);

        receive_uint32(socket, &size_dest);

        dest = (char*)calloc(size_dest, sizeof(char));

        real_path_dest = (char*)calloc(size_dest + sizeof("./files/"), sizeof(char));

        receive_data(socket, dest, size_dest);

        generateRealPath(dest, real_path_dest);

        if (access(real_path_src, F_OK) != 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        if (access(real_path_src, R_OK | W_OK) != 0) {
                send_uint32(socket, S_PERM_DENIED);
                return -1;
        }

        int fd = createFileWithDirectories(real_path_dest);

        if (fd < 0) {
            send_uint32(socket, S_UNDEFINED_ERR);
            return -1;
        }

        flock(fd, LOCK_EX);

        rename(real_path_src, real_path_dest);

        send_uint32(socket, S_SUCCES);

        update_file_list();

        log_operation("MOVE", src, NULL);

        flock(fd, LOCK_UN);

        close(fd);
        free(src);
        free(dest);
        free(real_path_dest);
        free(real_path_src);

        pthread_cond_signal(&cond_var_cont_exec);

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

        // printf("SALUT\n");

        if (access(real_path, F_OK) != 0) {
                send_uint32(socket, S_FILE_NOT_FOUND);
                return -1;
        }

        if (access(real_path, W_OK) != 0) {
                send_uint32(socket, S_PERM_DENIED);
                return -1;
        }

        if ((fd = open(real_path, O_RDWR)) < 0) {
                send_uint32(socket, S_UNDEFINED_ERR);
                return -1;
        }

        // printf("BEFORE LOCK\n");

        flock(fd, LOCK_EX);

        // printf("AFTER LOCK\n");

        lseek(fd, offset, SEEK_SET);

        write(fd, data, size_data);

        // printf("DATA WRITE\n");

        send_uint32(socket, S_SUCCES);

        // printf("In update\n");

        log_operation("UPDATE", path, NULL);

        flock(fd, LOCK_UN);

        close(fd);

        free(path);

        free(real_path);

        free(data);

        pthread_cond_signal(&cond_var_cont_exec);

        return 0;
}

int search(int socket)
{
        char word_to_find[MAX_WORD_LENGTH] = { 0 };

        uint32_t size_of_word = -1;

        char *list_of_paths = NULL;

        uint32_t size_of_list = 0;

        size_t offset = 0;

        pthread_mutex_lock(&mutex_freq_list);

        /* for (int i = 0; i < global_freq_list_size; i ++) {
                for (int j = 0; j < DEFAULT_NO_WORDS; j ++)
                        printf("<%d>\t%s\t%d\n", (j), global_freq_list[i].word_map[j].word, global_freq_list[i].word_map[j].frequency);
        } */

        receive_uint32(socket, &size_of_word);

        receive_data(socket, &word_to_find, size_of_word);

        // printf("%s\n", word_to_find);

        for (int i = 0; i < global_freq_list_size; i ++) {
                for (int j = 0; j < DEFAULT_NO_WORDS; j ++) {
                        // printf("%d %d\n", i, j);
                        if ( strcmp(word_to_find, global_freq_list[i].word_map[j].word) == 0) {
                                /* // printf("found the word in %s\n", global_freq_list[i].filename);
                                if (list_of_paths == NULL)
                                        list_of_paths = (char*)calloc(strlen(global_freq_list[i].filename) + 1, sizeof(char));
                                else
                                        list_of_paths = (char*)realloc(list_of_paths, strlen(list_of_paths) + strlen(global_freq_list[i].filename) + 1);

                                memcpy(list_of_paths + offset, global_freq_list[i].filename, strlen(global_freq_list[i].filename));
                                offset += strlen(global_freq_list[i].filename) + 1;
                                // list_of_paths[offset - 1] = '\0';
                                break; */
                                size_of_list += strlen(global_freq_list[i].filename) + 1;
                                break;
                        }
                        printf("%d %d\n", i, j);
                }
        }

        list_of_paths = (char*)calloc(size_of_list + 1, sizeof(char));

        for (int i = 0; i < global_freq_list_size; i ++) {
                for (int j = 0; j < DEFAULT_NO_WORDS; j ++)
                        if ( strcmp(word_to_find, global_freq_list[i].word_map[j].word) == 0) {
                                memcpy(list_of_paths + offset, global_freq_list[i].filename, strlen(global_freq_list[i].filename));
                                offset += strlen(global_freq_list[i].filename) + 1;
                                break;
                        }
        }


        pthread_mutex_unlock(&mutex_freq_list);

        send_uint32(socket, S_SUCCES);

        send_uint32(socket, size_of_list);

        send_data(socket, list_of_paths, offset);

        free(list_of_paths);

        log_operation("SEARCH", NULL, word_to_find);

        return 0;
}