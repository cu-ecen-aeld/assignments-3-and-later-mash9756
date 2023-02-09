#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define MS_TO_US (1000)

void* threadfunc(void* thread_param)
{
    printf("\nEntered thread, getting passed parameters...");
    int mutex_res = 0;
    int sleep_res = 0;
    // TODO:  wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    /* cast passed thread parameters to thread_data type for access to mutex */
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if(thread_func_args == NULL)
    {
        printf("\nPointer to thread parameters is NULL, exiting...");
        return thread_param;
    }

    if(thread_func_args->mutex == NULL)
    {
        printf("\nMutex points to NULL, exiting...");
        return thread_param;
    }

    if(thread_func_args->obtain_wait_ms < 0 || thread_func_args->release_wait_ms < 0)
    {
        printf("\nOne or both sleep durations are invalid.");
        return thread_param;
    }

    printf("\nWait before lock: %d", thread_func_args->obtain_wait_ms);
    printf("\nWait before unlock: %d", thread_func_args->release_wait_ms);

    printf("\nSleeping...");
    /* wait obtain_wait_ms */
    sleep_res = usleep((thread_func_args->obtain_wait_ms) * MS_TO_US);
    if(sleep_res != 0)
    {
        printf("Failed to sleep. Exiting.");
        //ERROR_LOG("Failed to sleep");
        return thread_param;
    }
    printf("\nWaking up, obtaining mutex...");

    mutex_res = pthread_mutex_lock(thread_func_args->mutex);
    if(mutex_res != 0)
    {
        printf("Failed to obtain mutex, error code %d", mutex_res);
        return thread_param;
    }
    printf("\nMutex obtained. Back to sleep...");

    /* wait release_wait_ms */
    sleep_res = usleep((thread_func_args->release_wait_ms) * MS_TO_US);
    if(sleep_res != 0)
    {
        printf("Failed to sleep");
        return thread_param;
    }
    printf("\nWaking up, releasing mutex...");

    mutex_res = pthread_mutex_unlock(thread_func_args->mutex);
    if(mutex_res != 0)
    {
        printf("Failed to release mutex, error code %d", mutex_res);
        return thread_param;
    }

    printf("\nMutex released, exiting...");
    thread_func_args->thread_complete_success = true;
    return (void *) thread_func_args;
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

    int create_res = 0;

    printf("\nAllocating thread memory...");
    /* allocate memory for thread instance */
    struct thread_data *thread_inst = (struct thread_data *) malloc(sizeof(struct thread_data*));
    if(thread_inst == NULL)
    {
        printf("malloc failed, NULL pointer returned");
        return false;
    }

    printf("\nAssignming thread data...");
    thread_inst->thread_complete_success = false;
    thread_inst->mutex = mutex;
    thread_inst->obtain_wait_ms = wait_to_obtain_ms;
    thread_inst->release_wait_ms = wait_to_release_ms;

    printf("\nCreating thread...");
    /* create thread */
    create_res = pthread_create(thread, NULL, threadfunc, (void *)thread_inst);
    if(create_res != 0)
    {
        printf("\nFailed to create thread, error ID: %d", create_res);
    }
    printf("\nThread Created Successfully!");

    return true;
}
