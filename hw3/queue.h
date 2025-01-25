#ifndef QUEUE_H_
#define QUEUE_H_

#include <sys/time.h>
#include <stdlib.h>

#include "segel.h"


typedef struct Node Node;
typedef struct queue queue;
queue* initializeQueue (int max_size);
int pop (queue* queue,  struct timeval *arrival_time_ptr);
int pop_by_val (queue* queue, int val);
int pop_by_index (queue* queue, int index);
int push (queue* queue, int value, struct timeval arrival_time);
int getSize(queue* queue);

#endif /* QUEUE_H_ */