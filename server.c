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

// Track number of active (processing) requests
static int active_count = 0;
// Store the maximum “additional” queue size (user-provided)
static int max_queue_size = 0;

void getargs(int *port, int *threads_num, int *queue_size, int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
}

// Worker thread: waits for pending requests, processes them, and tracks active_count
void *worker_thread(void *arg)
{
    // printf("[server] Worker thread %d started\n", *(int *)arg);
    int thread_id = *(int *)arg;
    threads_stats t = malloc(sizeof(struct Threads_stats));
    t->id = thread_id;
    t->stat_req = 0;
    t->dynm_req = 0;
    t->total_req = 0;

    while (1)
    {
        pthread_mutex_lock(&m);
        // Wait until there is at least one pending request
        while (is_empty(wait_queue))
        {
            pthread_cond_wait(&not_empty, &m);
        }
        // printf("[server] Worker thread %d processing request\n", thread_id);
        
        // Mark this thread as busy
        active_count++;
        
        // Dequeue the next pending request
        struct timeval arrival = get_head_arrival(wait_queue);
        int connfd = dequeue(wait_queue);

        // Signal to the acceptor that there is now room for more pending connections
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&m);

        // Record dispatch time
        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);

        // Handle the request
        requestHandle(connfd, arrival, dispatch, t, sharedLog);
        Close(connfd);

        // Once done, mark this thread as no longer busy
        pthread_mutex_lock(&m);
        active_count--;
        pthread_mutex_unlock(&m);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int port, threads_num, queue_max_size;
    getargs(&port, &threads_num, &queue_max_size, argc, argv);

    // Save the “additional” queue size parameter
    max_queue_size = queue_max_size;

    // Create the shared server log
    sharedLog = create_log();

    // Initialize queue with capacity = threads_num + queue_max_size
    wait_queue = queue_create(threads_num + queue_max_size);
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);

    // Start worker threads
    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    for (int i = 0; i < threads_num; i++)
    {
        int *tid = malloc(sizeof(int));
        *tid = i;
        pthread_create(&threads[i], NULL, worker_thread, tid);
    }

    // Begin listening on the specified port
    int listenfd = Open_listenfd(port);
    while (1)
    {
        struct sockaddr_in clientaddr;
        int clientlen = sizeof(clientaddr);
        int connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);

        struct timeval arrival;
        gettimeofday(&arrival, NULL);

        pthread_mutex_lock(&m);
        // Compute total = number of pending + number of active
        int total = queue_size(wait_queue) + active_count;

        // If total has reached the combined limit, drop this connection
        if (total >= threads_num + max_queue_size)
        {
            printf("[server] Dropping request—over capacity at %ld.%06ld\n", arrival.tv_sec, arrival.tv_usec);
            Close(connfd);
        }
        else
        {
            // Enqueue the new request; a worker will pick it immediately if available
            enqueue(wait_queue, connfd, arrival);
            pthread_cond_signal(&not_empty);
        }
        pthread_mutex_unlock(&m);
    }

    destroy_log(sharedLog);
    queue_destroy(wait_queue);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
    return 0;
}