/*
 * Write a program that uses fork, the child process should print hello and parent process should print goodbye.
 * Can you do this without using wait()?
 * */

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main() {
	printf("Parent pid: %d \n", (int) getpid());

	int forkResp = fork();
	if (forkResp < 0) {
		printf("Fork failed\n");
		return -1;
	} else if (forkResp == 0) {
		printf("Hello from child \n");
	} else {
		// parent proc
		wait(&forkResp);
		printf("Goodbye from parent \n");
	}
}
