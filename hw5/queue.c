/*
 * queue.c November 21, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

Queue *queue_create() {
	Queue *queue = (struct Queue*) malloc(sizeof(Queue));
	queue->front = NULL;
	queue->rear = NULL;
	queue->count = 0;
	return queue;
}

QueueNode *queue_node(int index) {
	QueueNode *node = (QueueNode*) malloc(sizeof(QueueNode));
	node->index = index;
	node->next = NULL;
	return node;
}

void queue_push(Queue *queue, int index) {
	QueueNode *node = queue_node(index);
	queue->count++;
	if (queue->rear == NULL) {
		queue->front = queue->rear = node;
		return;
	}
	queue->rear->next = node;
	queue->rear = node;
}

QueueNode *queue_pop(Queue *queue) {
	if (queue->front == NULL) return NULL;
	QueueNode *temp = queue->front;
	free(temp);
	queue->front = queue->front->next;
	if (queue->front == NULL) queue->rear = NULL;
	queue->count--;
	return temp;
}

bool queue_empty(Queue *queue) {
	if (queue->rear == NULL) return true;
	return false;
}

int queue_size(Queue *queue) {
	return queue->count;
}