#include "utils.h"

int send_data(int sockfd, const void *buffer, size_t bufsize)
{
    const char *pbuffer = (const char*) buffer;
    while (bufsize > 0)
    {
        int n = send(sockfd, pbuffer, bufsize, 0);
        if (n < 0) return -1;
        pbuffer += n;
        bufsize -= n;
    }
    return 0;
}

int receive_data(int sockfd, void *buffer, size_t bufsize)
{
    char *pbuffer = (char*) buffer;
    while (bufsize > 0)
    {
        int n = recv(sockfd, pbuffer, bufsize, 0);
        if (n <= 0) return n;
        pbuffer += n;
        bufsize -= n;
    }
    return 1;
}

int receive_data_to_file(int socket, int fd, size_t file_size)
{
    char buffer[1024];
    ssize_t bytesRead, bytesWritten;
    size_t totalBytesWritten = 0;

    while (totalBytesWritten < file_size)
    {
        bytesRead = read(socket, buffer, sizeof(buffer));
        if (bytesRead <= 0)
        {
            perror("receive_data");
            break;
        }

        bytesWritten = write(fd, buffer, bytesRead);
        if (bytesWritten <= 0)
        {
            perror("write");
            break;
        }

        totalBytesWritten += bytesWritten;
    }

    return 0;
}

int send_uint32(int sockfd, uint32_t data)
{
    data = htonl(data);
    return send_data(sockfd, &data, sizeof(data));
}

int receive_uint32(int sockfd, uint32_t *data)
{
    int ret = receive_data(sockfd, data, sizeof(*data));
    if (ret <= 0) *data = 0;
    else *data = ntohl(*data);
    return ret;
}

char* extractDirPath(const char *filePath) {
    char *dirPath = strdup(filePath);
    char *lastSlash = strrchr(dirPath, PATH_SEPARATOR);
    if (lastSlash != NULL) {
        *lastSlash = '\0';
    }
    return dirPath;
}

void _mkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

void createFileWithDirectories(const char *filePath)
{
    char *dirPath = extractDirPath(filePath);

    printf("%s\n", dirPath);

    _mkdir(dirPath);

    free(dirPath);

    int file = open(filePath, O_CREAT | O_EXCL, 0666);
    if (file != NULL)
    {
        fprintf(stderr, "File created: %s\n", filePath);
    }
    else
    {
        fprintf(stderr, "Error creating the file: %s\n", filePath);
    }
}
