#include <stdbool.h>
#include <sys/time.h>

typedef struct Queue *Queue;
typedef struct Node *Node;

Node node_create(int value, struct timeval arrival);
Queue queue_create(int size);
int queue_size(Queue queue);
bool queue_full(Queue queue);
bool queue_empty(Queue queue);
void enqueue(Queue queue, int value, struct timeval arrival);
struct timeval queue_head_arrival_time(Queue queue);
int dequeue(Queue queue);
void queue_destroy(Queue queue);
