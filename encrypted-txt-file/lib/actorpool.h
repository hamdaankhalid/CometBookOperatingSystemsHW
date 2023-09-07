#ifndef  ACTORPOOL
#define  ACTORPOOL

struct __threadPool;
typedef struct __threadPool ThreadPool;

int init_thread_pool(ThreadPool* pool, int n);

#endif
