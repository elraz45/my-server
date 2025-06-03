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

Node node_create(int data, struct timeval arrival)
{
    Node node = (Node)malloc(sizeof(*node));
    node->data = data;
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

bool is_full(Queue q)
{
    return q->current_size == q->max_size;
}

bool is_empty(Queue q)
{
    return q->current_size == 0;
}

void enqueue(Queue q, int data, struct timeval arrival)
{
    if (is_full(q))
        return;

    Node new_node = node_create(data, arrival);
    if (is_empty(q))
    {
        q->head = new_node;
        q->tail = new_node;
    }
    else
    {
        q->tail->next = new_node;
        q->tail = new_node;
    }

    q->current_size++;
}

struct timeval get_head_arrival(Queue q)
{
    if (is_empty(q))
        return (struct timeval){0};
    return q->head->arrival;
}

int dequeue(Queue q)
{
    if (is_empty(q))
        return -1;
    Node next_node = q->head->next;
    int data = q->head->data;
    free(q->head);
    if (next_node == NULL)
    {
        q->head = NULL;
        q->tail = NULL;
    }
    else
    {
        q->head = next_node;
    }
    q->current_size--;
    return data;
}

int queue_size(Queue q)
{
    return q->current_size;
}

void queue_destroy(Queue q)
{
    Node cur = q->head;
    Node next = NULL;
    while (cur)
    {
        next = cur->next;
        free(cur);
        cur = next;
    }

    free(q);
}
