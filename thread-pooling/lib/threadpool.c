#include "threadpool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_off_t.h>
#include <unistd.h>

// Every worker thread will run this function
// This function will run till it is signaled
// to be shutdown
void *worker_runner(ThreadPool *thread_pool) {
  int keep_running = 1;
  pthread_t tid;
  tid = pthread_self();

  printf("read lock acquire res %d \n",
         pthread_rwlock_rdlock(&thread_pool->exit_signal_lock));
  keep_running = thread_pool->exit_signal;
  printf("keep running is %d \n", keep_running);
  pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  // while not signal to exit has been set off
  // block on MPSC channel
  while (keep_running) {
    printf("Running worker thread with ID -> %lu \n", tid);
    if (sleep(1) > 0) {
      printf("Error has happened");
    }

    printf("read lock acquire res %d \n",
           pthread_rwlock_rdlock(&thread_pool->exit_signal_lock));
    keep_running = thread_pool->exit_signal;
    printf("keep running is %d \n", keep_running);
    pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  }

  printf("worker thread with ID exiting -> %lu \n", tid);
  pthread_exit(NULL);
}

// given a thread pool data obj pointer and num of threads
// intiailize a set of worker threads and store their metadata in
// obj pointer fields. Return 0 if successful else an Error Code enum
InitThreadPoolResult init_thread_pool(ThreadPool *thread_pool,
                                      int num_threads) {
  if (num_threads <= 0) {
    return INIT_THREAD_POOL_INVALID_NUM_THREADS;
  }
  
  if (pthread_rwlock_init(&thread_pool->exit_signal_lock, NULL) != 0) {
    return INIT_THREAD_POOL_RW_LOCK_ERR;
  }

  thread_pool->exit_signal = 1;
  thread_pool->num_threads = num_threads;

  thread_pool->workers = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  if (thread_pool->workers == NULL) {
    return INIT_THREAD_POOL_MEMORY_ERR;
  }

  for (int i = 0; i < num_threads; i++) {
    if (pthread_create(&thread_pool->workers[i], NULL,
                       (void *(*)(void *)) & worker_runner,
                       (void *)&thread_pool) != 0) {
      return INIT_THREAD_POOL_THREAD_CREATE_FAILED;
    }
  }

  return INIT_THREAD_POOL_SUCCESS;
};

DestroyThreadPoolResult destroy_thread_pool(ThreadPool *thread_pool) {

  // signal all threads to end
  pthread_rwlock_wrlock(&thread_pool->exit_signal_lock);
  thread_pool->exit_signal = 0;
  pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  // wait for threads to join
  for (int i = 0; i < thread_pool->num_threads; i++) {
    if (pthread_join(thread_pool->workers[i], NULL) != 0) {
      return DESTROY_THREAD_POOL_JOIN_FAIL;
    }
  }

  // free array of pthread_t
  free(thread_pool->workers);

  // destroy rw lock
  if (pthread_rwlock_destroy(&thread_pool->exit_signal_lock) != 0) {
    return DESTROY_THREAD_POOL_RW_LOCK_ERR;
  }

  return DESTROY_THREAD_POOL_SUCCESS;
}
