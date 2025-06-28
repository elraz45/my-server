#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "log.h"

// Opaque struct definition
struct Server_Log
{
    char *buffer; // Dynamic log buffer
    int size;     // Used size
    int capacity; // Allocated buffer size

    // Reader-writer synchronization
    pthread_mutex_t lock;
    pthread_cond_t readers_ok;
    pthread_cond_t writers_ok;

    int reader_count;    // Active readers
    int waiting_writers; // Writers waiting
    int writing;         // Is there a writer?
};

// Creates a new server log instance
server_log create_log()
{
    server_log log = (server_log)malloc(sizeof(struct Server_Log));
    if (!log)
        return NULL;

    log->capacity = 1024;
    log->size = 0;
    log->buffer = (char *)malloc(log->capacity);
    if (!log->buffer)
    {
        free(log);
        return NULL;
    }

    log->buffer[0] = '\0';
    log->reader_count = 0;
    log->waiting_writers = 0;
    log->writing = 0;

    if (pthread_mutex_init(&log->lock, NULL) != 0 ||
        pthread_cond_init(&log->readers_ok, NULL) != 0 ||
        pthread_cond_init(&log->writers_ok, NULL) != 0)
    {
        free(log->buffer);
        free(log);
        return NULL;
    }

    return log;
}

// Destroys and frees the log
void destroy_log(server_log log)
{
    if (!log)
        return;

    pthread_mutex_destroy(&log->lock);
    pthread_cond_destroy(&log->readers_ok);
    pthread_cond_destroy(&log->writers_ok);
    free(log->buffer);
    free(log);
}

// Returns full log content as dynamically allocated string (reader with priority rules)
int get_log(server_log log, char **dst)
{
    if (!log || !dst)
        return 0;

    pthread_mutex_lock(&log->lock);

    while (log->writing || log->waiting_writers > 0)
        pthread_cond_wait(&log->readers_ok, &log->lock);

    log->reader_count++;
    pthread_mutex_unlock(&log->lock);

    // Critical section for reader
    *dst = (char *)malloc(log->size + 1);
    if (!*dst)
        return 0;

    memcpy(*dst, log->buffer, log->size);
    (*dst)[log->size] = '\0';
    int copied_size = log->size;

    pthread_mutex_lock(&log->lock);
    log->reader_count--;
    if (log->reader_count == 0)
        pthread_cond_signal(&log->writers_ok);
    pthread_mutex_unlock(&log->lock);

    return copied_size;
}

// Appends a new entry to the log (single writer, prioritized)
void add_to_log(server_log log, const char *data, int data_len)
{
    if (!log || !data || data_len <= 0)
        return;

    pthread_mutex_lock(&log->lock);
    log->waiting_writers++;

    while (log->writing || log->reader_count > 0)
        pthread_cond_wait(&log->writers_ok, &log->lock);

    log->waiting_writers--;
    log->writing = 1;
    pthread_mutex_unlock(&log->lock);

    // Writer's data handling (single-writer enforced earlier)
    if (log->size + data_len >= log->capacity)
    {
        int new_capacity = log->capacity * 2;
        while (new_capacity < log->size + data_len)
            new_capacity *= 2;

        char *new_buffer = realloc(log->buffer, new_capacity);
        if (!new_buffer)
        {
            pthread_mutex_lock(&log->lock);
            log->writing = 0;
            pthread_cond_broadcast(&log->writers_ok);
            pthread_cond_broadcast(&log->readers_ok);
            pthread_mutex_unlock(&log->lock);
            return;
        }

        log->buffer = new_buffer;
        log->capacity = new_capacity;
    }

    memcpy(log->buffer + log->size, data, data_len);
    log->size += data_len;
    log->buffer[log->size] = '\0';

    pthread_mutex_lock(&log->lock);
    log->writing = 0;

    // Priority to writers
    if (log->waiting_writers > 0)
        pthread_cond_signal(&log->writers_ok);
    else
        pthread_cond_broadcast(&log->readers_ok);

    pthread_mutex_unlock(&log->lock);
}
