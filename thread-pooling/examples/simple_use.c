#include <threadpool.h>

#include <stdio.h>

int main() {
  ThreadPool tp;
  InitThreadPoolResult tp_init_res;

  tp_init_res = init_thread_pool(&tp, 10);

  printf("thread pool was created with result %d \n", tp_init_res);
  // ----- INITIALIZATION ---------

  sleep(3);

  printf("Requesting thread pool destruction\n");

  // ----- DESTRUCTION -------
  if (destroy_thread_pool(&tp) != DESTROY_THREAD_POOL_SUCCESS) {
    printf("Failed to destroy thread pool\n");
    return -1;
  }

  printf("Destroyed thread pool\n");
  return 0;
}
