#include "utils.h"

int move(const int socket, const char* src, const char* dest);

int update(const int socket, const char* filename, const uint32_t offset, const char* data);

int main()
{
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8017);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        connect(socketfd, (struct sockaddr*)(&addr), sizeof(addr));
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

                while (*current != '\0') {
                        printf("String: %s\n", current);
                        current += strlen(current) + 1;
                }
        }

        // valid test on download

/*         uint32_t status = -1;
        char path[] = "./downloads/salut.txt";
        send_uint32(socketfd, OP_DOWNLOAD);

        send_uint32(socketfd, sizeof(path));

        send_data(socketfd, path, sizeof(path));

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
        } */

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

        sendfile(socketfd, fd, NULL, size_to_send);

        receive_uint32(socketfd, &status);

        if (status != S_SUCCES) {
                printf("Error status: %x\n", status);
        }


        close(fd); */

        // test on delete

/*         send_uint32(socketfd, OP_DELETE);

        send_uint32(socketfd, sizeof("./bin/file_client.txt"));

        send_data(socketfd, "./bin/file_client.txt", sizeof("./bin/file_client.txt"));
 */

        // move

        move(socketfd ,"./downloads/salut.txt", "./salut.txt");

        // update

        printf("Update\n");
        update(socketfd, "./downloads/salut1.txt", 10, "TEXT MODIFICAT");
        sleep(1);
        printf("Update\n");
        update(socketfd, "./downloads/salut2.txt", 10, "TEXT MODIFICAT");
        sleep(1);
        printf("Move\n");
        move(socketfd, "abdc", "abcd");
        printf("Update\n");
        update(socketfd, "./downloads/salut1.txt", 10, "TEXT MODIFICAT");
        printf("Update\n");
        update(socketfd, "./downloads/salut1.txt", 20, "TEXT MODIFICAT1");


        close(socketfd);
        return 0;
}

int move(const int socket, const char* src, const char* dest)
{
        uint32_t status = -1;
        uint32_t size_src = -1;
        uint32_t size_dest = -1;

        size_src = strlen(src);
        size_dest = strlen(dest);

        if (size_src <= 0 || size_dest <= 0) {
                return -1;
        }

        send_uint32(socket, OP_MOVE);

        send_uint32(socket, size_src);

        send_data(socket, src, size_src);

        send_uint32(socket, size_dest);

        send_data(socket, dest, size_dest);

        receive_uint32(socket, &status);

        if (status != 0) {
                fprintf(stderr, "Error on move. %d\n", status);
                return -1;
        }

        return 0;
}

int update(const int socket, const char *filename, const uint32_t offset, const char *data)
{
        uint32_t status = -1;
        uint32_t size_path = -1;
        uint32_t size_data = -1;

        size_path = strlen(filename);
        size_data = strlen(data);

        send_uint32(socket, OP_UPDATE);

        send_uint32(socket, size_path);

        send_data(socket, filename, size_path);

        send_uint32(socket, offset);

        send_uint32(socket, size_data);

        send_data(socket, data, size_data);

        receive_uint32(socket, &status);

        if (status != 0) {
                fprintf(stderr, "Error on upload. %d\n", status);
                return -1;
        }

        return 0;
}
