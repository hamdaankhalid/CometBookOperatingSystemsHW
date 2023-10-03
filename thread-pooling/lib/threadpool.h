#ifndef THREADPOOL
#define THREADPOOL

#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>

#define MAX_BUFFER 10

typedef void *(*UserDefFunc_t)(void *);

typedef struct __task {
  UserDefFunc_t func;
  void *args;
  char uuid_str[37];
  sem_t *task_awaiter;
  void *task_result;
  size_t result_size;
} Task;

void new_task(Task *task, UserDefFunc_t func, void *args, void *task_result, size_t result_size);

void destroy_task(Task *task);

void await_task(Task *task);

typedef struct {
  int num_threads;

  // Read Write Exit signal lock and its exit signal flag
  pthread_rwlock_t exit_signal_lock;
  int exit_signal;
  pthread_t *workers;

  // Buffered Channel For workers and enqueuer func to use
  Task buffer[MAX_BUFFER];
  int buffer_fill;
  int buffer_use;
  sem_t empty;
  sem_t full;
  sem_t mutex; // semaphore mutex used to wrap the critical sections of put and
               // get calls to buffer
} ThreadPool;

typedef enum {
  INIT_THREAD_POOL_SUCCESS = 0,
  INIT_THREAD_POOL_INVALID_NUM_THREADS = -1,
  INIT_THREAD_POOL_MEMORY_ERR = -2,
  INIT_THREAD_POOL_THREAD_CREATE_FAILED = -3,
  INIT_THREAD_POOL_RW_LOCK_ERR = -4,
  INIT_THREAD_POOL_SEM_ERR = -5,
} InitThreadPoolResult;

InitThreadPoolResult init_thread_pool(ThreadPool *thread_pool, int num_threads);

typedef enum {
  ENQUEUE_TASK_SUCCESS = 0,
  ENQUEUE_TASK_SEM_ERR = -1,
} EnqueueTaskResponseCode;

typedef struct {
  EnqueueTaskResponseCode resp_code;
  sem_t *task_awaiter;
} EnqueueTaskResponse;

EnqueueTaskResponse enqueue_task(ThreadPool *pool, Task task);

typedef enum {
  DESTROY_THREAD_POOL_SUCCESS = 0,
  DESTROY_THREAD_POOL_JOIN_FAIL = -1,
  DESTROY_THREAD_POOL_RW_LOCK_ERR = -2,
} DestroyThreadPoolResult;

DestroyThreadPoolResult destroy_thread_pool(ThreadPool *thread_pool);

#endif
