#include <threadpool.h>

#include <stdio.h>
#include <time.h>

#define NUMBER_TASKS 10

typedef struct {
	int x;
	int y;
} AddArgs;

int add(AddArgs* args) {
	return args->x + args->y;
}

int main() {
  ThreadPool tp;
  InitThreadPoolResult tp_init_res;

  tp_init_res = init_thread_pool(&tp, 6);

  printf("thread pool was created with result %d \n", tp_init_res);

  if (tp_init_res != 0) {
    printf("Failed to initialize thread pool\n");
    return -1;
  }

  // ----- INITIALIZATION ---------
  int results[NUMBER_TASKS];

  printf("######## SEQUENTIAL ASYNC AWAIT #############\n");
  time_t sequential_start = time(NULL);
  Task sequential_tasks[NUMBER_TASKS];
  AddArgs args = {2, 3};
  for (int i = 0; i < NUMBER_TASKS; i++) {
    new_task(&sequential_tasks[i], &add, &args, &results[i], sizeof(int));
    enqueue_task(&tp, sequential_tasks[i]);
    await_task(&sequential_tasks[i]);
    destroy_task(&sequential_tasks[i]);
  }
  time_t sequential_end = time(NULL);
  double seq_diff = difftime(sequential_end, sequential_start);
  printf("---- Sequential Task Processing Completed In: %f -----\n", seq_diff);

  printf("######### CONCURRENT ASYNC AWAIT ############\n");
  time_t concurrent_start = time(NULL);
  Task concurrent_tasks[NUMBER_TASKS];
  for (int i = 0; i < NUMBER_TASKS; i++) {
    new_task(&concurrent_tasks[i], &add, &args, &results[i], sizeof(int));
    enqueue_task(&tp, concurrent_tasks[i]);
  }

  // await and destroy separately so the bottleneck is now the slowest task only
  for (int i = 0; i < NUMBER_TASKS; i++) {
    await_task(&concurrent_tasks[i]);
    destroy_task(&concurrent_tasks[i]);
  }
  time_t concurrent_end = time(NULL);
  double conc_diff = difftime(concurrent_end, concurrent_start);
  printf("---- Concurrent Task Processing Completed In: %f ---- \n", conc_diff);

  // ----- DESTRUCTION -------
  printf("Requesting thread pool destruction\n");

  if (destroy_thread_pool(&tp) != DESTROY_THREAD_POOL_SUCCESS) {
    printf("Failed to destroy thread pool\n");
    return -1;
  }

  printf("Destroyed thread pool\n");
  return 0;
}
