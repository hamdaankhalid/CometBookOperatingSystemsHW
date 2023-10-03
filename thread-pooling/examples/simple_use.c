#include <threadpool.h>

#include <stdio.h>

int main() {
  ThreadPool tp;
  InitThreadPoolResult tp_init_res;

  tp_init_res = init_thread_pool(&tp, 20);

  printf("thread pool was created with result %d \n", tp_init_res);

  if (tp_init_res != 0) {
    printf("Failed to initialize thread pool\n");
    return -1;
  }

  // ----- INITIALIZATION ---------

  // create 50 new tasks
  // kick them off in sequence and track how long it took to complete
  for (int i = 0; i < 230; i++) {
  }

  // destroy old 50 tasks

  // create another 50 tasks
  // kick them off in parallel and track how long it took to complete

  // ----- DESTRUCTION -------
  printf("Requesting thread pool destruction\n");

  if (destroy_thread_pool(&tp) != DESTROY_THREAD_POOL_SUCCESS) {
    printf("Failed to destroy thread pool\n");
    return -1;
  }

  printf("Destroyed thread pool\n");
  return 0;
}
