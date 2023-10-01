#include "threadpool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Every worker thread will run this function
// This function will run till it is signaled
// to be shutdown
void *worker_runner(void *args) {
  pthread_t tid;
  tid = pthread_self();
  // while not signal to exit has been set off
  // block on MPSC channel
  while (1) {
    printf("Running worker thread with ID -> %lu \n", tid);
    if (sleep(1) > 0) {
      printf("Error has happened");
    }
  }
  return NULL;
}

// given a thread pool data obj pointer and num of threads
// intiailize a set of worker threads and store their metadata in
// obj pointer fields. Return 0 if successful else an Error Code enum
InitThreadPoolResult init_thread_pool(ThreadPool *thread_pool,
                                      int num_threads) {
  int thread_creation_result;
  int thread_join_result;

  if (num_threads <= 0) {
    return INIT_THREAD_POOL_INVALID_NUM_THREADS;
  }
	
  if(pthread_rwlock_init(&thread_pool->exit_signal_lock, NULL) != 0) {
	  return INIT_THREAD_POOL_RW_LOCK_ERR;
  }

  thread_pool->num_threads = num_threads;

  thread_pool->workersGetDescriptionTags =
      (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  if (thread_pool->workersGetDescriptionTags == NULL) {
    return INIT_THREAD_POOL_MEMORY_ERR;
  }

  for (int i = 0; i < num_threads; i++) {
    thread_creation_result = pthread_create(
        &thread_pool->workersGetDescriptionTags[i], NULL, &worker_runner, NULL);

    if (thread_creation_result != 0) {
      return INIT_THREAD_POOL_THREAD_CREATE_FAILED;
    }
  }

  // debugging only
  for (int i = 0; i < num_threads; i++) {
    thread_join_result =
        pthread_join(thread_pool->workersGetDescriptionTags[i], NULL);

    if (thread_join_result != 0) {
      return INIT_THREAD_POOL_THREAD_CREATE_FAILED;
    }
  }
	
  return INIT_THREAD_POOL_SUCCESS;
};

DestroyThreadPoolResult destroy_thread_pool(ThreadPool* thread_pool) {
	// signal all threads to end
	// wait for threads to join
	// free array of pthread_t
	// destroy rw lock
}
