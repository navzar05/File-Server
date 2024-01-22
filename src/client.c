#include "utils.h"

struct sockaddr_in addr;


int list();

int download(const char* filename);

int upload(const char* path);

int delete(const char* filename);

int move(const char* src, const char* dest);

int update(const char* filename, const uint32_t offset, const char* data);

int search(const char* word);

int main()
{
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8023);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

/*         send_uint32(socketfd, OP_LIST);
        sleep(1);
        send_uint32(socketfd, OP_DOWNLOAD);
        sleep(1);
        send_uint32(socketfd, OP_SEARCH);
        sleep(1);
        send_uint32(socketfd, OP_MOVE);
        sleep(1);
        send_uint32(socketfd, OP_UPDATE);
        sleep(1);
        send_uint32(socketfd, OP_UPLOAD);
        sleep(1);
        send_uint32(socketfd, 0x23); */

        // test on list

        // list(socketfd);


        // search("Ana");

        list();

        download("./downloads/salut.txt");

        delete("./Makefile");

        list();

        upload("./Makefile");

        update("/bin/file_client.txt", 13, " Ana ");

        search("Ana");

        // update(socketfd, "./test.txt", 0, "Ana");

        // search(socketfd, "Ana");

        // valid test on download
/*
        sleep(1);

        download(socketfd, "./salut.txt");

        download(socketfd, "./salut.txt");
 */
        // download(socketfd, "./salut.txt");

        // test on upload

        /* uint32_t status = -1;
        uint32_t size_to_send = -1;
        char path[] = "./bin/file_client.txt";
        send_uint32(socketfd, OP_UPLOAD);

        send_uint32(socketfd, sizeof(path));

        send_data(socketfd, path, sizeof(path));

        int fd = open(path, O_RDONLY);

        if (fd < 0) {
                printf("failed to open file.\n");
        }

        struct stat buf;
        memset(&buf, 0, sizeof(struct stat));
        fstat(fd, &buf);
        size_to_send = buf.st_size;

        printf("size to send = %d\n", size_to_send);

        send_uint32(socketfd, size_to_send);

        if (sendfile(socketfd, fd, NULL, size_to_send) < 0)
                printf("ERROR ON SEND FILE\n");

        receive_uint32(socketfd, &status);

        if (status != S_SUCCES) {
                printf("Error status: %x\n", status);
        }


        close(fd);
 */
        // test on delete

/*         send_uint32(socketfd, OP_DELETE);

        send_uint32(socketfd, sizeof("./bin/file_client.txt"));

        send_data(socketfd, "./bin/file_client.txt", sizeof("./bin/file_client.txt"));
 */

        // move

        // update(socketfd, "./salut.txt", 200, "Ana");

        // move(socketfd ,"./downloads/salut1.txt", "./abcd/sss/salut.txt");

        // update

        /* printf("Update\n");
        update(socketfd, "./downloads/salut1.txt", 10, "TEXT MODIFICAT");
        sleep(1);
        printf("Update\n");
        update(socketfd, "./downloads/salut2.txt", 10, "TEXT MODIFICAT");
        sleep(1);
        printf("Update\n");
        update(socketfd, "./downloads/salut.txt", 10, "TEXT MODIFICAT");
        printf("Update\n");
        update(socketfd, "./downloads/salut1.txt", 20, "TEXT MODIFICAT1"); */

        return 0;
}

int list()
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        send_uint32(socketfd, OP_LIST);
        receive_uint32(socketfd, &status);

        if (status != 0)
                fprintf(stderr, "Test on list failed.\n");
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

int download(const char* filename)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        send_uint32(socketfd, OP_DOWNLOAD);

        printf("%s %d\n", filename, strlen(filename));

        send_uint32(socketfd, strlen(filename));

        send_data(socketfd, filename, strlen(filename));

        receive_uint32(socketfd, &status);

        if (status == S_SUCCES) {
                uint32_t size = 0;
                char* buffer = NULL;

                receive_uint32(socketfd, &size);
                printf("size = %d\n", size);

                buffer = (char*)calloc(size, sizeof(char));

                receive_data(socketfd, buffer, size);

                printf("buffer: %s\n", buffer);
        }
        else {
                fprintf(stderr, "Failed to download");
        }
        close(socketfd);
        return 0;
}

int upload(const char* path)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        uint32_t size_to_send = -1;

        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        uint32_t status = -1;
        send_uint32(socketfd, OP_UPLOAD);

        send_uint32(socketfd, strlen(path));

        send_data(socketfd, path, strlen(path));

        int fd = open(path, O_RDONLY);

        if (fd < 0) {
                printf("failed to open file.\n");
        }

        struct stat buf;
        memset(&buf, 0, sizeof(struct stat));
        fstat(fd, &buf);
        size_to_send = buf.st_size;

        printf("size to send = %d\n", size_to_send);

        send_uint32(socketfd, size_to_send);

        if (sendfile(socketfd, fd, NULL, size_to_send) < 0)
                printf("ERROR ON SEND FILE\n");

        receive_uint32(socketfd, &status);

        if (status != S_SUCCES) {
                printf("Error status: %x\n", status);
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

        send_uint32(socketfd, OP_DELETE);

        send_uint32(socketfd, strlen(filename));

        send_data(socketfd, filename, strlen(filename));

        receive_uint32(socketfd, &status);

        if (status != 0) {
                fprintf(stderr, "Error on delete. %d\n", status);
        }
        close(socketfd);
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

        if (size_src <= 0 || size_dest <= 0) {
                return -1;
        }

        send_uint32(socketfd, OP_MOVE);

        send_uint32(socketfd, size_src);

        send_data(socketfd, src, size_src);

        send_uint32(socketfd, size_dest);

        send_data(socketfd, dest, size_dest);

        receive_uint32(socketfd, &status);

        if (status != 0) {
                fprintf(stderr, "Error on move. %d\n", status);
        }
        close(socketfd);
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
                fprintf(stderr, "Error on upload. %d\n", status);
        }
        close(socketfd);
        return 0;
}

int search(const char *word)
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
        int status = -1;
        int size = -1;
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