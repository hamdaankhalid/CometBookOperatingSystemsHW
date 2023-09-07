#include "actorpool.h"

#include <pthread.h>
#include <stdlib.h>

// ****************** Thread Management Utils *************

/*
 * A sleeping set of threads that are all listening to a channel for inputs.
 * on an input to the channel the threads will then proceed to run a function
 * that was user defined and passed to the struct
 * The components we need to make for this to work include the following:
 * - Executor Func -> listens to a channel till a signal to quit is deployed,
 *     if signal to quit is recvd we return.
 * 
 * - Thread pool -> Hold the threads and synchronization primitives
 * 
 * - Await Job Result -> Pauses job execution till the sent job completes
 *
 * - Await Jobs -> Given a set of jobs, pauyse execution till they complete
 * */

struct __threadPool {
	int num_threads;
	pthread_t* thread_pools;
};

int init_thread_pool(ThreadPool* pool, int n) {
	if (n < 1) {
		return -1;
	}

	pool->num_threads = n;
	pool->thread_pools = (pthread_t*)malloc(sizeof(pthread_t) * n);
	if (pool->thread_pools == NULL) {
		return -1;
	}

	for (int i = 0; i < n; i++) {
		pool->thread_pools[i];
	}

	return 0;
}

int async_enqueue(ThreadPool* pool, void* (func)(void*), void* args) {
	// add the message to the channel
	// return the job id
	return 0;
}

// sync wait for the job with jid to have a result
void* await(ThreadPool* pool, int jid) {
	// spin lock till the result is added to the thread
	// or till we have an error thrown by a job
	return NULL;
}

// executor expects that a user defined function is perfffeeecttt and never errors out
int executor() {
	// this wil be a thread listens to the shit
	/*
	 * while (read_channel() = data)  {
	 *   take the data and run it on the user defined function
	 *   take the result and store it somewhere and place this
	 *   result in a hashmap for the job id.
	 * }
	 * */
	return 0;
}

// **************** Non-Blocking Single Producer Multiple Consumer Buffered Channel Implementation ***************
// need read, write, and signal exit
