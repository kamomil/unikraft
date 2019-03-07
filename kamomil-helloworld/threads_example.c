#include <stdio.h>

#include <stdlib.h>

/* Import user configuration: */
#include <uk/config.h>
#include <uk/sched.h>
#include <time.h>
//#include <thread.h>

void thread_func(void* p){
  struct timespec req = {30,400};

  printf("thread_func: about to sleep in a loop\n");
  while(nanosleep(&req,&req)<0){
    printf("thread_func: left to sleep %ld seconds, ERR = %d\n",req.tv_sec,errno);
  }
  printf("thread_func: BYE!\n");
 
  return;
}

int main(int argc, char *argv[])
{
	printf("Hello world!\n");

#if APPHELLOWORLD_PRINTARGS
	int i;

	printf("Arguments: ");
	for (i=0; i<argc; ++i)
		printf(" \"%s\"", argv[i]);
	printf("\n");
#endif
	struct uk_thread *thread = uk_sched_thread_create(uk_sched_get_default(), "second thread",thread_func,NULL);
	int i = 0;
	for(int i=0;i<30;i++){
		struct timespec req = {2,30};
		nanosleep(&req,NULL);
		if(i%5 == 0){
		  printf("main thread: waking second thread:\n");
		  uk_thread_wake(thread);
		}
	}
}
