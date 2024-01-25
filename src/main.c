#include "server.h"

extern int listenSock;
extern pthread_mutex_t index_mutex;
pthread_key_t commSock;
extern pthread_t tids[MAX_NO_THREADS];
extern size_t index_of_threads;



void *thread_client(void *args)
{
        int *aux = (int*)args;
        pthread_setspecific(commSock, aux);
        int sock = *((int*)pthread_getspecific(commSock));
        uint32_t op_code = -1;

        receive_uint32(sock, &op_code);

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
                search(sock);
                break;
        default:
                printf("Unknown operation code: %x\n", op_code);
                break;
        }


        pthread_mutex_lock(&index_mutex);
        if (index_of_threads > 0)
                index_of_threads--;
        pthread_mutex_unlock(&index_mutex);

        if (close(sock) < 0)
                perror("Failed to close connection");
        printf("Thread finished.\n");
        return NULL;
}

void clean_auxsock(void *args)
{
        free((int*)args);
}

int main()
{
        int *auxsock;
        auxsock = (int*)calloc(1, sizeof(int));
        pthread_cleanup_push(clean_auxsock, auxsock);

        if (server_init(&listenSock) < 0) {
                fprintf(stderr, "Error on server init.\n");
                return -1;
        }

        printf("listenSock: %d\n", listenSock);

        pthread_key_create(&commSock, NULL);

        while (1) {
                (*auxsock) = accept(listenSock, NULL, NULL);


                if (index_of_threads == MAX_NO_THREADS) {
                        fprintf(stderr, "Maximum number of threads reached.\n");
                        send_uint32((*auxsock), S_SERVER_BUSY);
                        close((*auxsock));
                        continue;
                }

                // printf("New socket: %d\n", (*auxsock));

                if ((*auxsock) < 0) {
                        perror("accept error");
                        continue;
                }


                pthread_create(&tids[index_of_threads], NULL, thread_client, auxsock);

                pthread_mutex_lock(&index_mutex);
                index_of_threads++;
                pthread_mutex_unlock(&index_mutex);
                // pthread_detach(tids[index_of_threads]);


        }
        pthread_key_delete(commSock);

        pthread_cleanup_pop(1);

        return 0;
}