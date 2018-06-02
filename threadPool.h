

#ifndef __THREAD_POOL__
#define __THREAD_POOL__


#include <sys/types.h>
#include "osqueue.h"
#define STDERR 2
#define ERROR "Error in system call\n"
#define MEMORY_ERROR "Error in allocate memory\n"


enum State {RUNNING, DESTROY, END, AFTER_JOIN};

typedef struct task{
    void (*function)(void *);
    void *argument;
} Task;

typedef struct thread_pool
{
    int num_threads;
    int stop;
    pthread_t *threads;
    OSQueue* queue;
    enum State enumState;
    pthread_mutex_t qlock;
    pthread_mutex_t qEndlock;
    pthread_cond_t q_empty;

    void (*executeTasks)(void *);

}ThreadPool;


/********************************************
 * tpCreate function-create the ThreadPool
 * @param numOfThreads
 * @return pointer to ThreadPool
 ********************************************/
ThreadPool* tpCreate(int numOfThreads);

/*********************************************
 * tpDestroy function- destroy the ThreadPool
 * and free all the tasks
 * @param threadPool
 * @param shouldWaitForTasks
 *********************************************/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

/*********************************************************
 * tpInsertTask function- insert task to the ThreadPool
 * @param threadPool
 * @param computeFunc
 * @param param
 * @return 0 is succses
 *         -1 in faild
 ********************************************************/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

/*******************************************************
 * executeTasks function- handles the thread life
 * @param arg
 *******************************************************/
void executeTasks(void *arg);

/**
 * execute function.
 * @param arg
 * @return
 */
void* execute(void *arg);


/***********************************************
 * prints error and exits with -1 the program.
 ***********************************************/
void fail();

/***********************************************
 * prints error in allocate memory
 * and exits with -1 the program.
 ***********************************************/
void failMemory();

#endif
