#include "utils.h"

struct sockaddr_in addr;


int list();

int download(const char* filename);

int upload(const char* path);

int delete(const char* filename);

int move(const char* src, const char* dest);

int update(const char* filename, const uint32_t offset, const char* data);

int search(const char* word);

void print_error(u_int32_t error_code);

int main()
{
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8023);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        // update("./downloads/salut.txt", 0, "MODIFICARE");
        //search("dada");

        /* for (int i = 0; i < 1000; i ++) {
                update("./downloads/salut.txt", 0, "MODIFICARE");
                list();
                usleep(100);
        }

        delete("./downloads/salut1.txt"); */

        list();

        // delete("./Makefile");

        list();

        upload("./Makefile");

        download("./Makefile");

        update("/bin/file_client.txt", 13, " #MODIFICARE#");

        search("Ana");

        return 0;
}

void print_error(u_int32_t error_code)
{
        switch (error_code)
        {
        case S_BAD_ARGS:
                fprintf(stderr, "Bad arguments.\n");
                break;

        case S_FILE_NOT_FOUND:
                fprintf(stderr, "File not found.\n");
                break;

        case S_PERM_DENIED:
                fprintf(stderr, "Permmsion denied.\n");
                break;

        case S_OUT_OF_MEM:
                fprintf(stderr, "Out of memory.\n");
                break;

        case S_SERVER_BUSY:
                fprintf(stderr, "Server busy.\n");
                break;

        case S_UNKNOWN_OP:
                fprintf(stderr, "Unkown operation.\n");
                break;

        case S_UNDEFINED_ERR:
                fprintf(stderr, "Undefined error.\n");
                break;

        default:
                fprintf(stderr, "Bad error code.\n");
                break;
        }
}

int list()
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        send_uint32(socketfd, OP_LIST);
        receive_uint32(socketfd, &status);

        if (status != 0)
                print_error(status);
        else {
                uint32_t size;
                char* buffer = NULL;
                receive_uint32(socketfd, &size);
                printf("size: %d\n", size);
                buffer = (char*)calloc(size, sizeof(char));
                read(socketfd, buffer, size);

                const char *current = buffer;

                printf("Files:\n");

                while (*current != '\0') {
                        printf("\t%s\n", current + sizeof("./files"));
                        current += strlen(current) + 1;
                }
        }
        close(socketfd);
        return 0;
}

const char* get_filename_from_path(const char* path) {
        const char* filename = strrchr(path, '/');
        if (filename == NULL) {
                filename = path;

        } else {

                filename++;

        }

        return filename;
}

int download(const char* filename)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        send_uint32(socketfd, OP_DOWNLOAD);

        uint32_t size = strlen(filename);

        if (size == 0) {
                fprintf(stderr, "filename is empty\n");
                size = -1;
        }

        printf("%s %d\n", filename, size);

        send_uint32(socketfd, size);

        if (size != -1)
                send_data(socketfd, filename, size);

        receive_uint32(socketfd, &status);

        if (status == S_SUCCES) {
                uint32_t size = 0;
                char* buffer = NULL;

                receive_uint32(socketfd, &size);
                printf("size = %d\n", size);

                buffer = (char*)calloc(size, sizeof(char));

                receive_data(socketfd, buffer, size);

                printf("buffer: %s\n", buffer);

                int fd = -1;
                int i = 0;
                char filename_download[MAX_BUFFER] = { 0 };
                char filename_download_unique[MAX_BUFFER] = { 0 };

                strcpy(filename_download, get_filename_from_path(filename));
                strcpy(filename_download_unique, filename_download);

                while (fd < 0) {

                        fd = open(filename_download_unique, O_EXCL | O_CREAT | O_WRONLY, 0600);
                        if (fd < 0) {
                                // fprintf(stderr, "File cannot be opened\n");
                                snprintf(filename_download_unique, MAX_BUFFER + sizeof(int), "%s(%d)",
                                filename_download, i);
                                i ++;
                        }
                }

                 write(fd, buffer, size);


        }
        else {
                print_error(status);
        }
        close(socketfd);
        return 0;
}

int upload(const char* path)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        uint32_t size_to_send = -1;
        int fd = -1;

        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;

        uint32_t size = strlen(path);

        if (size == 0) {
                fprintf(stderr, "Filename is empty.\n");
                size = -1;
        }

        send_uint32(socketfd, OP_UPLOAD);

        send_uint32(socketfd, size);

        if (size != -1) {
                send_data(socketfd, path, size);
                fd = open(path, O_RDONLY);
        }

        if (fd >= 0) {

                struct stat buf;
                memset(&buf, 0, sizeof(struct stat));
                fstat(fd, &buf);
                size_to_send = buf.st_size;

                printf("size to send = %d\n", size_to_send);

                send_uint32(socketfd, size_to_send);

                if (sendfile(socketfd, fd, NULL, size_to_send) < 0)
                        printf("ERROR ON SEND FILE\n");

        }

        receive_uint32(socketfd, &status);

        if (status != S_SUCCES) {
                print_error(status);
        }

        close(fd);

        close(socketfd);

        return 0;
}

int delete(const char* filename)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;

        if (strlen(filename) == 0) {
                send_uint32(socketfd, OP_DELETE);

                send_uint32(socketfd, -1);

                receive_uint32(socketfd, &status);

        }
        else {
                send_uint32(socketfd, OP_DELETE);

                send_uint32(socketfd, strlen(filename));

                send_data(socketfd, filename, strlen(filename));

                receive_uint32(socketfd, &status);
        }

        close(socketfd);

        if (status != 0) {
                print_error(status);
                return -1;
        }
        return 0;
}

int move(const char* src, const char* dest)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        uint32_t size_src = -1;
        uint32_t size_dest = -1;

        size_src = strlen(src);
        size_dest = strlen(dest);

        send_uint32(socketfd, OP_MOVE);

        if (size_src <= 0 || size_dest <= 0) {
                send_uint32(socketfd, -1);

                receive_uint32(socketfd, &status);
        }

        else {
                send_uint32(socketfd, size_src);

                send_data(socketfd, src, size_src);

                send_uint32(socketfd, size_dest);

                send_data(socketfd, dest, size_dest);

                receive_uint32(socketfd, &status);
        }

        close(socketfd);

        if (status != 0) {
                print_error(status);
                return -1;
        }

        return 0;
}

int update(const char *filename, const uint32_t offset, const char *data)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        uint32_t size_path = -1;
        uint32_t size_data = -1;

        size_path = strlen(filename);
        size_data = strlen(data);

        send_uint32(socketfd, OP_UPDATE);

        send_uint32(socketfd, size_path);

        send_data(socketfd, filename, size_path);

        send_uint32(socketfd, offset);

        send_uint32(socketfd, size_data);

        send_data(socketfd, data, size_data);

        receive_uint32(socketfd, &status);

        if (status != 0) {
                print_error(status);
        }
        close(socketfd);
        return 0;
}

int search(const char *word)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        uint32_t size = -1;
        char *paths = NULL;

        send_uint32(socketfd, OP_SEARCH);

        send_uint32(socketfd, strlen(word));

        send_data(socketfd, word, strlen(word));

        receive_uint32(socketfd, &status);

        receive_uint32(socketfd, &size);

        paths = (char*)calloc(size, sizeof(char));

        receive_data(socketfd, paths, size);

        printf("Word \"%s\" found in this files:\n", word);

        const char *current = paths;

                while (*current != '\0') {
                printf("\t%s\n", current);
                current += strlen(current) + 1;
        }

        close(socketfd);
        return 0;
}