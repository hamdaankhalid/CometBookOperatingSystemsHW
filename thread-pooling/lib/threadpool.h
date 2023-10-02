#ifndef THREADPOOL
#define THREADPOOL

#include <pthread.h>

typedef struct {
  int num_threads;
  // Read Write Exit signal lock and its exit signal flag
  pthread_rwlock_t exit_signal_lock;
  int exit_signal;
  pthread_t *workers;
} ThreadPool;

typedef enum {
  INIT_THREAD_POOL_SUCCESS = 0,
  INIT_THREAD_POOL_INVALID_NUM_THREADS = -1,
  INIT_THREAD_POOL_MEMORY_ERR = -2,
  INIT_THREAD_POOL_THREAD_CREATE_FAILED = -3,
  INIT_THREAD_POOL_RW_LOCK_ERR = -4,
} InitThreadPoolResult;

InitThreadPoolResult init_thread_pool(ThreadPool *thread_pool, int num_threads);

typedef enum {
  DESTROY_THREAD_POOL_SUCCESS = 0,
  DESTROY_THREAD_POOL_JOIN_FAIL = -1,
  DESTROY_THREAD_POOL_RW_LOCK_ERR = -2,
} DestroyThreadPoolResult;

DestroyThreadPoolResult destroy_thread_pool(ThreadPool *thread_pool);

#endif
