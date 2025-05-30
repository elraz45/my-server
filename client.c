/*
 * client.c: A very, very primitive HTTP client.
 *
 * Example usage:
 *      ./client www.example.com 80 / GET 5
 *
 * This client sends multiple HTTP requests to a server using threads.
 */

#include "segel.h"
#include <pthread.h>

// Sends an HTTP request to the server using the given socket
void clientSend(int fd, char *filename, char *method)
{
    char buf[MAXLINE];
    char hostname[MAXLINE];

    Gethostname(hostname, MAXLINE); // Get the client's hostname

    // Form the HTTP request line and headers
    sprintf(buf, "%s %s HTTP/1.1\n", method, filename);
    sprintf(buf, "%shost: %s\n\r\n", buf, hostname);

    printf("Request:\n%s\n", buf); // Display the request for debugging

    // Send the request to the server
    Rio_writen(fd, buf, strlen(buf));
}

// Reads and prints the server's HTTP response
void clientPrint(int fd)
{
    rio_t rio;
    char buf[MAXBUF];
    int length = 0;
    int n;

    // Initialize buffered input
    Rio_readinitb(&rio, fd);

    // --- Read and print HTTP headers ---
    n = Rio_readlineb(&rio, buf, MAXBUF);
    while (strcmp(buf, "\r\n") && (n > 0))
    {
        printf("Header: %s", buf);

        // Try to parse Content-Length header
        if (sscanf(buf, "Content-Length: %d ", &length) == 1)
        {
            printf("Length = %d\n", length);
        }

        n = Rio_readlineb(&rio, buf, MAXBUF);
    }

    // --- Read and print HTTP body ---
    n = Rio_readlineb(&rio, buf, MAXBUF);
    while (n > 0)
    {
        printf("%s", buf);
        n = Rio_readlineb(&rio, buf, MAXBUF);
    }
}

// Struct to pass args to each thread
typedef struct
{
    char *host;
    int port;
    char *filename;
    char *method;
    int id;
} client_args;

void *client_thread(void *arg)
{
    client_args *args = (client_args *)arg;
    int clientfd = Open_clientfd(args->host, args->port);
    if (clientfd < 0)
    {
        fprintf(stderr, "[Thread %d] Error connecting to %s:%d\n", args->id, args->host, args->port);
        return NULL;
    }

    printf("[Thread %d] Connected to server. clientfd = %d\n", args->id, clientfd);
    clientSend(clientfd, args->filename, args->method);
    clientPrint(clientfd);
    Close(clientfd);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <host> <port> <filename> <method> <num_threads>\n", argv[0]);
        exit(1);
    }

    char *host = argv[1];
    int port = atoi(argv[2]);
    char *filename = argv[3];
    char *method = argv[4];
    int num_threads = atoi(argv[5]);

    pthread_t threads[num_threads];
    client_args args[num_threads];

    for (int i = 0; i < num_threads; i++)
    {
        args[i].host = host;
        args[i].port = port;
        args[i].filename = filename;
        args[i].method = method;
        args[i].id = i + 1;
        pthread_create(&threads[i], NULL, client_thread, &args[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}