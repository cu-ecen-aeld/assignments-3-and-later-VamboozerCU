#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    bool bPassedLock = false;
    bool bPassedUnlock = false;

    //printf("wait_to_obtain_ms = %d\n", thread_func_args->wait_to_obtain_ms);
    //printf("wait_to_release_ms = %d\n", thread_func_args->wait_to_release_ms);

    if(thread_func_args->wait_to_obtain_ms > 0)
        sleep((float)thread_func_args->wait_to_obtain_ms/1000.0);
    
    bPassedLock = (bool)pthread_mutex_lock(thread_func_args->mutex);
    //printf("LOCK = %d\n", (short)bPassedLock);

    if(thread_func_args->wait_to_release_ms > 0)
        sleep((float)thread_func_args->wait_to_release_ms/1000.0);

    bPassedUnlock = (bool)pthread_mutex_unlock(thread_func_args->mutex);
    //printf("UNLOCK = %d\n", (short)bPassedUnlock);

    if(!bPassedLock && !bPassedUnlock){
        thread_func_args->thread_complete_success = true;
    }
    else{
        thread_func_args->thread_complete_success = false;
    }
    //printf("OUT = %d\n", thread_func_args->thread_complete_success);

    return (void*)thread_func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    //printf("******************** Start Test *******************\n");
    
    struct thread_data *LocalThreadData = malloc(sizeof(struct thread_data));
    LocalThreadData->mutex = mutex;
    LocalThreadData->wait_to_obtain_ms = wait_to_obtain_ms;
    LocalThreadData->wait_to_release_ms = wait_to_release_ms;
    LocalThreadData->thread_complete_success = false;

    //printf("wait_to_obtain_ms = %d\n", LocalThreadData->wait_to_obtain_ms);
    //printf("wait_to_release_ms = %d\n", LocalThreadData->wait_to_release_ms);

    bool test;
    short nThread1 = (short)*thread;
    //printf("thread = %d\n", (short)*thread);
    test = (bool)pthread_create(thread, NULL, threadfunc, LocalThreadData);
    short nThread2 = (short)*thread;
    //printf("thread = %d\n", (short)*thread);
    
    //pthread_join(*thread, NULL);

    if((nThread1 != nThread2) && (test == 0)){
        //printf("PASS\n");
        return true;
    }
    //printf("FAIL\n");
    return false;
}
