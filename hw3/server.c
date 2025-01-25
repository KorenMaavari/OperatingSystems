#include "segel.h"
#include "request.h"
#include "queue.h"
#include "threads_stats.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>


// TODO : check about free


// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too

queue* waiting_queue = NULL;
queue* worker_queue = NULL;

pthread_mutex_t m;
pthread_cond_t c;
pthread_cond_t c_main;

int work_size = 0;
int wait_size = 0;



void getargs(int *port, int *thread_num, int *queue_size, char *schedalg,
 int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *thread_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    strcpy(schedalg, argv[4]);
    
}

void* manageThreads(void *args){

    struct timeval arrival_time, work_time, dispatch_time;
    threads_stats stats = (threads_stats)args;
    while(1)
    {
        pthread_mutex_lock(&m);
        while(wait_size == 0){
            pthread_cond_wait(&c, &m);
        }
        int connfd = pop(waiting_queue, &arrival_time);
        wait_size--;
        work_size++;
        //push(worker_queue, connfd, arrival_time);
        pthread_mutex_unlock(&m);

        gettimeofday(&work_time, NULL);
        timersub(&work_time, &arrival_time, &dispatch_time);

        requestHandle(connfd, arrival_time, dispatch_time, stats);
        Close(connfd);

        pthread_mutex_lock(&m);
        //pop_by_val(worker_queue, connfd);
        work_size--;
        pthread_cond_signal(&c_main);
        pthread_mutex_unlock(&m);

    }
    return NULL;
}

void handleRequest(int connfd, struct timeval arrival_time)
{
    push(waiting_queue, connfd, arrival_time);
    wait_size++;
    pthread_cond_signal(&c);
}

void handleBlock(int connfd, struct timeval arrival_time, int max_size)
{
    while (wait_size + work_size >= max_size) 
    {
        pthread_cond_wait(&c_main, &m);
    }
    handleRequest(connfd, arrival_time);
}

void handleRandom(int connfd, struct timeval arrival_time)
{
    if(wait_size == 0){
        Close(connfd);
    }
    else
    {
        int drop = 0;
        srand(time(NULL));
        if(wait_size % 2 == 0){
            drop = wait_size / 2;
        }
        else{
            drop = (wait_size + 1) / 2;
        }
        for(int i=0; i < drop; i++){
            int index = rand() % wait_size;
            int value = pop_by_index(waiting_queue, index);
            wait_size--;
            Close(value);
        }
        handleRequest(connfd,arrival_time);
    }
}

void handleBF(int connfd, struct timeval arrival_time)
{
    // Block until the queue is empty and no threads are handling requests
    while (wait_size + work_size > 0) {
        pthread_cond_wait(&c_main, &m);
    }
    // After the queue is empty and no threads are handling requests, drop the new request
    Close(connfd); // Close the socket to drop the new request
}

void handleDH(int connfd, struct timeval arrival_time)
{
    // Drop the oldest request in the queue
    struct timeval tmp_time;

    if (wait_size > 0) {
        int value = pop(waiting_queue, &tmp_time);
        wait_size--;
        Close(value);
        handleRequest(connfd, arrival_time);
    }
    else{
        Close(connfd);
    }

}

void handleDT(int connfd){
    Close(connfd);
}

   
int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    int thread_num, max_size;
    char schedalg[7]; // including null termin


    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
    pthread_cond_init(&c_main, NULL);


    getargs(&port, &thread_num, &max_size, schedalg, argc, argv);

    waiting_queue = initializeQueue(max_size);
    worker_queue = initializeQueue(max_size);

    // 
    // HW3: Create some threads...
    //

    pthread_t *worker_threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t)); 

    for (int i=0; i<thread_num; i++){
        threads_stats stats = (threads_stats)malloc(sizeof(threads_stats));
        initStats(stats, i);
        pthread_create(&worker_threads[i], NULL, manageThreads, (void *)stats);
    }
    struct timeval arrival_time;
    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&arrival_time, NULL);

        pthread_mutex_lock(&m);
        if (work_size + wait_size >= max_size) {
            if (strcmp(schedalg, "dt") == 0) {
                handleDT(connfd);
            } 
            else if (strcmp(schedalg, "dh") == 0) {
                handleDH(connfd, arrival_time);
            } 
            else if (strcmp(schedalg, "bf") == 0) {
              handleBF(connfd, arrival_time);
            }
            else if(strcmp(schedalg, "block")==0){
                handleBlock(connfd, arrival_time, max_size);
            }
            else if(strcmp(schedalg, "random")==0){
                handleRandom(connfd, arrival_time);
            }
        }
        else{
            handleRequest(connfd, arrival_time);
        }

        pthread_mutex_unlock(&m);

    }

    return 0;

}


 
