#include <stdbool.h>
#include <sys/time.h>

typedef struct Queue *Queue;
typedef struct Node *Node;

Node node_create(int data, struct timeval arrival);
Queue queue_create(int size);
int queue_size(Queue queue);
bool is_full(Queue queue);
bool is_empty(Queue queue);
void enqueue(Queue queue, int data, struct timeval arrival);
struct timeval get_head_arrival(Queue queue);
int dequeue(Queue queue);
void queue_destroy(Queue queue);
