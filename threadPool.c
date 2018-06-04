
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "threadPool.h"


ThreadPool* tpCreate(int numOfThreads){

    if (numOfThreads <= 0) {
        return NULL;
    }
    ThreadPool *pool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        failMemory();
    }
    pool->threads = (pthread_t*) malloc (sizeof(pthread_t) * numOfThreads);
    if(!pool->threads) {
        failMemory();
    }
    //initialize struct variables
    pool->num_threads = numOfThreads;
    pool->queue=osCreateQueue();
    pool->executeTasks=executeTasks;
    pool->enumState=RUNNING;
    pool->stop=0;
    //initialize mutex and condition variables
    if(pthread_mutex_init(&pool->qlock,NULL)||pthread_mutex_init(&pool->qEndlock,NULL)||
       pthread_cond_init(&(pool->q_empty),NULL) ) {
        fprintf(stderr, "Mutex initiation error!\n");
        return NULL;
    }

    //make threads
    int i;
    for (i = 0;i < numOfThreads;i++) {
        if(pthread_create(&(pool->threads[i]),NULL,execute,pool)) {
            fprintf(stderr, "Thread initiation error!\n");
            return NULL;
        }
    }
    return pool;
}


void* execute(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    pool->executeTasks(arg);
}

void executeTasks(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    while (pool->enumState!=END) {
        if(pool->enumState==DESTROY && osIsQueueEmpty(pool->queue)) {
            break;
        }else if (osIsQueueEmpty(pool->queue) && (pool->enumState==RUNNING
                                                  || pool->enumState==AFTER_JOIN)) {
            pthread_mutex_lock(&pool->qlock);
            pthread_cond_wait(&(pool->q_empty), &(pool->qlock)); //wait until there are tasks in queue
        }else {
            //the queue is not empty: lock the queue before a Task is executed
            pthread_mutex_lock(&pool->qlock);
        }
        //the queue was not empty: continue to execute the tasks
        if (pool->enumState != END) {
            //queue is empty
            if (osIsQueueEmpty(pool->queue)) {
                pthread_mutex_unlock(&pool->qlock);
            } else { //have tasks in the queue
                Task *task = osDequeue(pool->queue);
                pthread_mutex_unlock(&pool->qlock);
                task->function(task->argument);
                free(task);
            }
            //if all the tasks are finished and need to destroy
            if (pool->enumState == AFTER_JOIN) {
                break;
            }
        }
    }
}


int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void*), void* param) {

    if (threadPool->stop) {
        return -1;
    } else {
        //allocating memory for the new Task
        Task* newTask = (Task *) malloc(sizeof(Task));
        if (newTask == NULL) {
                fail();
        } else {
            //setting the fields for the Task
            newTask->function = computeFunc;
            newTask->argument = param;

            pthread_mutex_lock(&threadPool->qlock); //lock before adding to the queue
            osEnqueue(threadPool->queue, newTask);  //insert the Task to the queue
            if (pthread_cond_signal(&threadPool->q_empty) != 0) {
                fail();
            }
            pthread_mutex_unlock(&threadPool->qlock); //unlock before adding to the queue
            return 0; //success
        }
    }
}



void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
    pthread_mutex_lock(&threadPool->qEndlock);
    if (threadPool->stop) {
        return;
    }
    pthread_mutex_unlock(&threadPool->qEndlock);

    pthread_mutex_lock(&threadPool->qlock);
    if (pthread_cond_broadcast(&threadPool->q_empty) > 0
        || pthread_mutex_unlock(&threadPool->qlock) > 0) {
        fail();
    }
do {
    //if dont have tasks in the queue
    if (!shouldWaitForTasks) {
        threadPool->enumState = AFTER_JOIN;
    } else {
        threadPool->enumState = DESTROY;
    }
    pthread_mutex_lock(&threadPool->qEndlock);
    threadPool->stop = 1;
    pthread_cond_broadcast(&threadPool->q_empty);
    pthread_mutex_unlock(&threadPool->qEndlock);
    int i;
    int numOfThreads = threadPool->num_threads;
    //joining the threads
    for (i = 0; i < numOfThreads; ++i) {
        pthread_join(threadPool->threads[i], NULL);
    }
    threadPool->enumState = END;


    while (!osIsQueueEmpty(threadPool->queue)) {
        Task *task = osDequeue(threadPool->queue);
        free(task);
    }

    //destroy and free
    osDestroyQueue(threadPool->queue);
    free(threadPool->threads);
    free(threadPool);
    pthread_mutex_destroy(&threadPool->qlock);
    pthread_mutex_destroy(&threadPool->qEndlock);
} while(0);

}



void fail() {
    write(STDERR, ERROR, sizeof(ERROR)-1);
    exit(-1);
}

void failMemory() {
    write(STDERR, MEMORY_ERROR, sizeof(MEMORY_ERROR)-1);
    exit(-1);
}