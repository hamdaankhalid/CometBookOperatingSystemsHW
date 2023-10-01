#ifndef THREADPOOL
#define THREADPOOL

#include <pthread.h>

typedef enum {
	THREAD_IDLE,
	THREAD_BUSY,
};

typedef struct {
  int num_threads;
  pthread_t *workersGetDescriptionTags;
  // Read Write Exit signal lock and its exit signal flag
  pthread_rwlock_t exit_signal_lock;
  int exit_signal; 
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
} DestroyThreadPoolResult;

DestroyThreadPoolResult destroy_thread_pool(ThreadPool* thread_pool);

#endif
