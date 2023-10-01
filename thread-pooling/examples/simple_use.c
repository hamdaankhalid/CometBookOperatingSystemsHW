#include <threadpool.h>

#include <stdio.h>

int main() {
  ThreadPool tp;
  InitThreadPoolResult tp_init_res;

  tp_init_res = init_thread_pool(&tp, 3);

  printf("tp was created with result %d \n", tp_init_res);
}
