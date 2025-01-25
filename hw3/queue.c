#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "queue.h"


const int FAIL = -1;

typedef struct Node {
    struct timeval arrival_time;
    int value;
    struct Node* previous;
    struct Node* next;
} Node;

typedef struct queue{
    int size;
    int max_size;
    Node* tail;
    Node* head;
} queue;

queue* initializeQueue (int max_size) {
    queue* newQueue = (queue*)malloc(sizeof(queue));
    newQueue->size = 0;
    newQueue->tail = NULL;
    newQueue->head = NULL;
    newQueue->max_size = max_size;
    return newQueue;
}

int push (queue* queue, int value, struct timeval arrival_time) {
    // if(queue->size == queue->max_size ){
    //     return FAIL;
    // }
    Node* newNode = (Node*)malloc(sizeof(Node));
    // if (newNode == NULL) {
    //     Close(value);
    //     return FAIL;
    // }
    newNode->value = value; 
    newNode->next = queue->tail;
    newNode->previous = NULL;
    newNode->arrival_time = arrival_time;
     if (queue->size == 0) {
        queue->head = newNode;
        queue->tail = newNode;
    }
    else {
        queue->tail->previous = newNode;
        queue->tail = newNode;
    }
   
    queue->size++;
    return value;
};



int pop (queue* queue, struct timeval *arrival_time_ptr) {
    if(queue->size == 0){
        return FAIL;
    }
    Node* toDelete = queue->head;
    int value = queue->head->value;
    *arrival_time_ptr = queue->head->arrival_time;
    queue->head = queue->head->previous;
    if (queue->size == 1) {
        queue->tail = NULL;
    }
    else {
        queue->head->next = NULL;
    }
    free (toDelete);
    queue->size--;
    return value;
};


int pop_by_val (queue* queue, int val) {
    Node* curr = queue->tail;
    Node* toDelete = NULL;
    while(curr){
        if(curr->value == val){
            toDelete = curr;
            break;
        }
        else{
            curr = curr->next;
        }
    }
    if(curr == NULL){
        return FAIL;
    }
  
    int value = toDelete->value;
     if (queue->size == 1) {
        queue->tail = NULL;
        queue->head = NULL;
    }
    
    else if(toDelete->value == queue->head->value){
         queue->head = queue->head->previous;
         queue->head->next = NULL;
        
    }
    else if (toDelete->value == queue->tail->value)
    {
        queue->tail = queue->tail->next;
        queue->tail->previous = NULL;
    }
    else
    {
        toDelete->previous->next = toDelete->next;
        toDelete->next->previous = toDelete->previous;
    }
    free (toDelete);
    queue->size--;
    return value;
};

int pop_by_index (queue* queue, int index) {
    // assuming that the index is possible
    Node* toDelete = queue->tail;
    for(int i =0; i< index; i++){
        toDelete = toDelete->next;
    }
    int value = toDelete->value;
     if (queue->size == 1) {
        queue->tail = NULL;
        queue->head = NULL;
    }
    
    else if(toDelete->value == queue->head->value){
         queue->head = queue->head->previous;
         queue->head->next = NULL;
        
    }
    else if (toDelete->value == queue->tail->value)
    {
        queue->tail = queue->tail->next;
        queue->tail->previous = NULL;
    }
    else
    {
        toDelete->previous->next = toDelete->next;
        toDelete->next->previous = toDelete->previous;
    }
    free (toDelete);
    queue->size--;
    return value;
};

int getSize(queue* queue){
    return queue->size;
}


void deleteQueue (queue* queue) {
    struct timeval arrival_time;
    int size = queue->size;
    for (int i=0 ; i<size ; ++i) {
        pop(queue, &arrival_time); 
    }
}