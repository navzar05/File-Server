#include "server.h"

int listenSock = 0;
pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_key_t commSock;
pthread_t tids[MAX_NO_THREADS] = { 0 };
size_t tid_index = 0;

void *thread_client(void *args)
{
        int sock = 0;
        int hold_connection = 1;

        pthread_setspecific(commSock, args);
        sock = *((int*)pthread_getspecific(commSock));

        while(hold_connection) {
                uint32_t op_code = -1;
                if (receive_uint32(sock, &op_code) <= 0)
                        break;

                switch (op_code) {
                case OP_LIST:
                        printf("Operation: List\n");
                        list(sock);
                        break;
                case OP_DOWNLOAD:
                        printf("Operation: Download\n");
                        download(sock);
                        break;
                case OP_UPLOAD:
                        printf("Operation: Upload\n");
                        upload(sock);
                        break;
                case OP_DELETE:
                        printf("Operation: Delete\n");
                        delete(sock);
                        break;
                case OP_MOVE:
                        printf("Operation: Move\n");
                        move(sock);
                        break;
                case OP_UPDATE:
                        printf("Operation: Update\n");
                        update(sock);
                        break;
                case OP_SEARCH:
                        printf("Operation: Search\n");
                        break;
                default:
                        printf("Unknown operation code: %x\n", op_code);
                        hold_connection = 0;
                        break;
                }

        }

        pthread_mutex_lock(&index_mutex);
        if (tid_index > 0)
                tid_index--;
        pthread_mutex_unlock(&index_mutex);

        close(sock);
        return NULL;
}


int main()
{
        if (server_init(&listenSock) < 0) {
                fprintf(stderr, "Error on server init.\n");
                return -1;
        }

        printf("listenSock: %d\n", listenSock);

        pthread_key_create(&commSock, NULL);

        while (1) {
                int auxsock = accept(listenSock, NULL, NULL);

                if (tid_index == MAX_NO_THREADS) {
                        fprintf(stderr, "Maximum number of threads reached.\n");
                        send_uint32(auxsock, S_SERVER_BUSY);
                        continue;
                }

                printf("New socket: %d\n", auxsock);

                if (auxsock < 0) {
                        perror("accept error");
                        return -1;
                }

                pthread_create(&tids[tid_index], NULL, thread_client, &auxsock);
                pthread_detach(tids[tid_index]);
                tid_index++;
        }

        return 0;
}