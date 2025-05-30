#include <stdio.h>
#include "queue.h"

#include <stdlib.h>

struct Queue
{
    int max_size;
    int current_size;
    Node head;
    Node tail;
};

struct Node
{
    int data;
    struct timeval arrival;
    Node next;
};

Node node_create(int value, struct timeval arrival)
{
    Node node = (Node)malloc(sizeof(*node));
    node->data = value;
    node->arrival = arrival;
    node->next = NULL;
    return node;
}

Queue queue_create(int size)
{
    Queue queue = (Queue)malloc(sizeof(*queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->current_size = 0;
    queue->max_size = size;

    return queue;
}

bool queue_full(Queue queue)
{
    if (queue->current_size == queue->max_size)
        return true;
    else
        return false;
}

bool queue_empty(Queue queue)
{
    return queue->current_size == 0;
}

void enqueue(Queue queue, int value, struct timeval arrival)
{
    // drop tail policy
    if (queue_full(queue))
        return;

    Node new_node = node_create(value, arrival);
    if (queue_empty(queue))
    {
        queue->head = new_node;
        queue->tail = new_node;
    }
    else
    {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    queue->current_size++;
}

struct timeval queue_head_arrival_time(Queue queue)
{
    if (queue_empty(queue))
        return (struct timeval){0};
    return queue->head->arrival;
}

int dequeue(Queue queue)
{
    if (queue_empty(queue))
        return -1;
    Node temp = queue->head->next;
    int value = queue->head->data;
    free(queue->head);
    if (temp == NULL)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else
    {
        queue->head = temp;
    }
    queue->current_size--;
    return value;
}

int queue_size(Queue queue)
{
    return queue->current_size;
}

void queue_destroy(Queue queue)
{
    Node current = queue->head;
    Node next = NULL;
    while (current)
    {
        next = current->next;
        free(current);
        current = next;
    }

    free(queue);
}
