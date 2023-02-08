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
    int mutex_res = 0;
    // TODO:  wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    /* cast passed thread parameters to thread_data type for access to mutex */
    struct thread_data *thread_func_args = (struct thread_data *) thread_param;

    /* wait obtain_wait_ms */
    mutex_res = pthread_mutex_lock(&thread_func_args->mutex);
    /* wait release_wait_ms */
    mutex_res = pthread_mutex_unlock(&thread_func_args->mutex);

    return thread_param;
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

    /* allocate memory for thread instance */
    struct thread_data *thread_inst = malloc(sizeof(struct thread_data));

/* do we need a seperate struct for parameters? */
    thread_inst->obtain_wait_ms = wait_to_obtain_ms;
    thread_inst->release_wait_ms = wait_to_release_ms;

    /* create thread */
    pthread_create(thread_inst->thread_ID, NULL, threadfunc, &thread);
    pthread_join(thread_inst->thread_ID, thread_inst->retval);
    /* release memory allocated for thread instance */
    free(thread_inst);

/* should we return false here, or is that only on failure or something? */
    return false;
}
