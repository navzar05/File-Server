#include "utils.h"
#define MAX_PATH 1024


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
    char *lastSlash = strrchr(dirPath, '/');
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

int createFileWithDirectories(const char *filePath)
{
    char *dirPath = extractDirPath(filePath);

    // printf("%s\n", dirPath);

    _mkdir(dirPath);

    free(dirPath);

    int file = open(filePath, O_CREAT | O_EXCL | O_RDWR, 0666);

    if (file >= 0)
    {
        fprintf(stderr, "File created: %s\n", filePath);
        return file;
    }
    else
    {
        fprintf(stderr, "Error creating the file: %s\n", filePath);
        return -1;
    }
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