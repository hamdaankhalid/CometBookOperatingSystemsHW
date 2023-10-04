#include <threadpool.h>

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define NUMBER_TASKS 40

typedef struct {
	int x;
	int y;
} AddArgs;

void add(AddArgs* args, int* out) {
    int milliseconds = 30 * 1000;
    usleep(milliseconds);
	*out = args->x + args->y;
}

int main() {
  ThreadPool tp;
  InitThreadPoolResult tp_init_res;

  tp_init_res = init_thread_pool(&tp, 15);

  printf("thread pool was created with result %d \n", tp_init_res);

  if (tp_init_res != 0) {
    printf("Failed to initialize thread pool\n");
    return -1;
  }

  // ----- INITIALIZATION ---------
  int seq_results[NUMBER_TASKS];

  printf("######## SEQUENTIAL ASYNC AWAIT #############\n");
  clock_t sequential_start = clock();
  Task sequential_tasks[NUMBER_TASKS];
  AddArgs args = {2, 3};
  for (int i = 0; i < NUMBER_TASKS; i++) {
    new_task(&sequential_tasks[i], (UserDefFunc_t)&add, &args, &seq_results[i], sizeof(int), FALSE);
    enqueue_task(&tp, sequential_tasks[i]);
    await_task(&sequential_tasks[i]);
    // test correctness
    assert(seq_results[i] == 5);
    destroy_task(&sequential_tasks[i]);
  }
  clock_t sequential_end = clock();
  double seq_diff = ((double) (sequential_end - sequential_start)) / CLOCKS_PER_SEC;
  printf("---- (Cpu Clock time) Sequential Task Processing Completed In: %f -----\n", seq_diff);


  int con_results[NUMBER_TASKS];
  printf("######### CONCURRENT ASYNC AWAIT ############\n");
  AddArgs con_args = {5, 3};
  clock_t conc_start = clock();
  Task concurrent_tasks[NUMBER_TASKS];
  for (int i = 0; i < NUMBER_TASKS; i++) {
    new_task(&concurrent_tasks[i], (UserDefFunc_t) &add, &con_args, &con_results[i], sizeof(int), FALSE);
    enqueue_task(&tp, concurrent_tasks[i]);
  }

  // await and destroy separately so the bottleneck is now the slowest task only
  for (int i = 0; i < NUMBER_TASKS; i++) {
    await_task(&concurrent_tasks[i]);
    assert(con_results[i] == 8);
    destroy_task(&concurrent_tasks[i]);
  }
  clock_t conc_end = clock();
  double conc_diff = ((double) (conc_end- conc_start)) / CLOCKS_PER_SEC;
  printf("---- (Cpu Clock time) Concurrent Task Processing Completed In: %f ---- \n", conc_diff);

  // ----- DESTRUCTION -------
  printf("Requesting thread pool destruction\n");

  if (destroy_thread_pool(&tp) != DESTROY_THREAD_POOL_SUCCESS) {
    printf("Failed to destroy thread pool\n");
    return -1;
  }

  printf("Destroyed thread pool\n");
  return 0;
}
