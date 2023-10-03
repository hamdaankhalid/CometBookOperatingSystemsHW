#include "threadpool.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>

#define RET_ON_FAIL(expr, resp)                                                \
  if (expr != 0) {                                                             \
    return resp;                                                               \
  }

// Macro based logger so that in non debug mode we are not incurring cost
// of extra instructions
#define DEBUG 0

// Variadic argument based logging macro
// Using macros makes sure non debugging
// code has no performance impact
#ifndef DEBUG
#define LOG(...) ;
#else
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOG(...)                                                               \
  pthread_mutex_lock(&log_mutex);                                              \
  printf(__VA_ARGS__);                                                         \
  pthread_mutex_unlock(&log_mutex);
#endif

void *worker_runner(ThreadPool *thread_pool);

// given a thread pool data obj pointer and num of threads
// intiailize a set of worker threads and store their metadata in
// obj pointer fields. Return 0 if successful else an Error Code enum
InitThreadPoolResult init_thread_pool(ThreadPool *thread_pool,
                                      int num_threads) {
  if (num_threads < 1) {
    return INIT_THREAD_POOL_INVALID_NUM_THREADS;
  }

  RET_ON_FAIL(pthread_rwlock_init(&thread_pool->exit_signal_lock, NULL),
              INIT_THREAD_POOL_RW_LOCK_ERR)

  RET_ON_FAIL(pthread_rwlock_wrlock(&thread_pool->exit_signal_lock),
              INIT_THREAD_POOL_RW_LOCK_ERR)

  thread_pool->exit_signal = 1;
  pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  thread_pool->num_threads = num_threads;

  /*
   * This sempahore is used by producer to check if the buffer
   * can be added to. It is initialized at max capacity,
   * each call to wait on this will decrement it if it is above 0,
   * each call to post will increment it.
   * If at 0 this call will be blocked.
   * Our producer will make a call to wait on this buffer to make
   * sure that if the buffer is full and no consumer has picked up
   * the task, the producer call will sit blocked, till a consumer picks it up
   */
  if (sem_init(&thread_pool->empty, 0, MAX_BUFFER) != 0) {
    pthread_rwlock_destroy(&thread_pool->exit_signal_lock);
    return INIT_THREAD_POOL_SEM_ERR;
  }

  /*
   * This semaphore is used by consumer to check if there is a job
   * to be picked up. It starts at 0, and is incrmented by the producer,
   * and waited on by the consumer.
   * */
  if (sem_init(&thread_pool->full, 0, 0) != 0) {
    sem_destroy(&thread_pool->empty);
    pthread_rwlock_destroy(&thread_pool->exit_signal_lock);
    return INIT_THREAD_POOL_SEM_ERR;
  }

  /*
   * This sempahore is a binary lock used to wrap the critical section
   * of interacting with buffer channel data by producer and consumer
   * */
  if (sem_init(&thread_pool->mutex, 0, 1) != 0) {
    sem_destroy(&thread_pool->empty);
    sem_destroy(&thread_pool->full);
    pthread_rwlock_destroy(&thread_pool->exit_signal_lock);
    return INIT_THREAD_POOL_SEM_ERR;
  }

  thread_pool->buffer_fill = 0;
  thread_pool->buffer_use = 0;

  thread_pool->workers = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  if (thread_pool->workers == NULL) {
    return INIT_THREAD_POOL_MEMORY_ERR;
  }

  for (int i = 0; i < num_threads; i++) {
    if (pthread_create(&thread_pool->workers[i], NULL,
                       (void *(*)(void *)) & worker_runner,
                       (void *)thread_pool) != 0) {
      free(thread_pool->workers);
      return INIT_THREAD_POOL_THREAD_CREATE_FAILED;
    }
  }

  return INIT_THREAD_POOL_SUCCESS;
};

// ctor/initializer for new task
void new_task(Task *task, UserDefFunc_t func, void *args, void *task_result) {
  task->func = func;
  task->args = args;
  task->task_result = task_result;

  uuid_t uuid;
  uuid_generate(uuid);
  uuid_unparse_lower(uuid, task->uuid_str);

  // start off awaiter semaphore with 0 so any calls to wait block the calling
  // thread
  task->task_awaiter = (sem_t *)malloc(sizeof(sem_t));
  sem_init(task->task_awaiter, 0, 0);
}

// dtor
void destroy_task(Task *task) {
  sem_destroy(task->task_awaiter);
  free(task->task_awaiter);
}

// blocks till the task is completed and result is
// filled
void await_task(Task *task) { sem_wait(task->task_awaiter); }

// NOTE: For the utilities below I am completely okay with returning
// the object by value since the object solely holds pointers, so the copy
// on return is cheap.

// utitlity to add task to buffer channel
void put(ThreadPool *pool, Task task) {
  pool->buffer[pool->buffer_fill] = task;
  pool->buffer_fill = (pool->buffer_fill + 1) % MAX_BUFFER;
}

// utitlity to get a task from buffer channel
Task get(ThreadPool *pool) {
  Task tmp = pool->buffer[pool->buffer_use];
  pool->buffer_use = (pool->buffer_use + 1) % MAX_BUFFER;
  return tmp;
}

/*
 * Used to enqueue an async task, this may block if the buffered channel
 * is full.
 * */
EnqueueTaskResponse enqueue_task(ThreadPool *pool, Task task) {
  LOG("PRODUCER: Enqueuing task\n")

  EnqueueTaskResponse enq_resp;
  enq_resp.resp_code = ENQUEUE_TASK_SEM_ERR;
  enq_resp.task_awaiter = NULL;

  RET_ON_FAIL(sem_wait(&pool->empty), enq_resp)
  RET_ON_FAIL(sem_wait(&pool->mutex), enq_resp)
  put(pool, task);
  RET_ON_FAIL(sem_post(&pool->mutex), enq_resp)
  RET_ON_FAIL(sem_post(&pool->full), enq_resp)

  LOG("PRODUCER: Task enqueued\n")
  enq_resp.resp_code = ENQUEUE_TASK_SUCCESS;
  return enq_resp;
}

// Every worker thread will run this function
// This function will run till it is signaled
// to be shutdown
void *worker_runner(ThreadPool *thread_pool) {
  int keep_running;
  pthread_t tid;
  tid = pthread_self();

  if (pthread_rwlock_rdlock(&thread_pool->exit_signal_lock) != 0) {
    pthread_exit(NULL);
  }
  keep_running = thread_pool->exit_signal;
  pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  // while not signal to exit has been set off
  // block on MPSC channel
  while (keep_running) {
    LOG("WORKER: Running thread with ID -> %lu \n", tid)

    sem_wait(&thread_pool->full);
    sem_wait(&thread_pool->mutex);
    Task task = get(thread_pool);
    sem_post(&thread_pool->mutex);
    sem_post(&thread_pool->empty);

    LOG("WORKER: %lu recvd task with id %s \n", tid, task.uuid_str)

    // fake processing
    sleep(1);

    sem_post(task.task_awaiter);

    if (pthread_rwlock_rdlock(&thread_pool->exit_signal_lock) != 0) {
      pthread_exit(NULL);
    }
    keep_running = thread_pool->exit_signal;
    pthread_rwlock_unlock(&thread_pool->exit_signal_lock);
  }

  LOG("WORKER: %lu exiting \n", tid)
  pthread_exit(NULL);
}

DestroyThreadPoolResult destroy_thread_pool(ThreadPool *thread_pool) {
  // signal all threads to end
  pthread_rwlock_wrlock(&thread_pool->exit_signal_lock);
  thread_pool->exit_signal = 0;
  pthread_rwlock_unlock(&thread_pool->exit_signal_lock);

  // wait for threads to join
  for (int i = 0; i < thread_pool->num_threads; i++) {
    RET_ON_FAIL(pthread_join(thread_pool->workers[i], NULL),
                DESTROY_THREAD_POOL_JOIN_FAIL)
  }

  // free array of pthread_t
  free(thread_pool->workers);

  // destroy rw lock
  RET_ON_FAIL(pthread_rwlock_destroy(&thread_pool->exit_signal_lock),
              DESTROY_THREAD_POOL_RW_LOCK_ERR)

  sem_destroy(&thread_pool->empty);
  sem_destroy(&thread_pool->full);
  sem_destroy(&thread_pool->mutex);

#ifdef DEBUG
  pthread_mutex_destroy(&log_mutex);
#endif
}
