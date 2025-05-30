#include "segel.h"
#include "request.h"
#include "log.h"
#include "queue.h"
#include <pthread.h>

pthread_mutex_t m;
pthread_cond_t not_empty;
pthread_cond_t not_full;

Queue wait_queue = NULL;
server_log sharedLog;

void getargs(int *port, int *thread_num, int *queue_size, int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *thread_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
}

// Thread function for the worker pool
void *worker_thread(void *arg)
{
    int thread_id = *(int *)arg;
    threads_stats t = malloc(sizeof(struct Threads_stats));
    t->id = thread_id;
    t->stat_req = 0;
    t->dynm_req = 0;
    t->total_req = 0;

    while (1)
    {
        pthread_mutex_lock(&m);

        while (is_empty(wait_queue))
        {
            pthread_cond_wait(&not_empty, &m);
        }

        struct timeval arrival = get_head_arrival(wait_queue);
        int connfd = dequeue(wait_queue);

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&m);

        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);

        requestHandle(connfd, arrival, dispatch, t, sharedLog);
        Close(connfd);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int port, threads_num, queue_max_size;
    getargs(&port, &threads_num, &queue_max_size, argc, argv);

    // Create the shared log
    sharedLog = create_log();

    // Init queue and synchronization
    wait_queue = queue_create(queue_max_size);
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);

    // Thread pool
    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    for (int i = 0; i < threads_num; i++)
    {
        int *tid = malloc(sizeof(int));
        *tid = i;
        pthread_create(&threads[i], NULL, worker_thread, tid);
    }

    // Start listening
    int listenfd = Open_listenfd(port);
    while (1)
    {
        struct sockaddr_in clientaddr;
        int clientlen = sizeof(clientaddr);
        int connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);

        struct timeval arrival;
        gettimeofday(&arrival, NULL);

        pthread_mutex_lock(&m);

        /* not drop tail -

        while (queue_full(wait_queue))
        {
            pthread_cond_wait(&not_full, &m);
        }
        enqueue(wait_queue, connfd, arrival);

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&m);

        */

        if (is_full(wait_queue))
        {
            // Drop-tail
            // printf("[server] Dropping request—over capacity at %ld.%06ld\n", arrival.tv_sec, arrival.tv_usec);
            Close(connfd);
        }
        else
        {
            enqueue(wait_queue, connfd, arrival);
            pthread_cond_signal(&not_empty);
        }
        pthread_mutex_unlock(&m);
    }

    // Cleanup
    destroy_log(sharedLog);
    queue_destroy(wait_queue);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}