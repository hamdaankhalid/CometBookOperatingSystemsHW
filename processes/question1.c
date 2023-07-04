/*
 * Write a program that calls fork(). Before calling fork have the main process access a cariable, and set its value to something. what value is the variable in child process? what happens to the variable when both child and parent process make changes to value of x.
 */

#include <unistd.h>
#include <stdio.h>

int main() {
	int x = 69;

	printf("The Parent Process with pid: %d, has x set to 69 at first \n", (int) getpid());

	int forkedResp = fork();

	if (forkedResp < 0) {
		printf("Failed to create child process \n");
		return -1;
	} else if (forkedResp == 0) {
		// this means we are inside the child process
		x = 420;
		printf("I the child process with pid: %d have changed the value of x to %d \n", (int)getpid(), x);
	} else {
		// forkedResp is PID of child, and this is returned to parent
		printf("I the parent process with id: %d have created the child with pid: %d. The value of x for me is %d \n", (int) getpid(), forkedResp, x);
	}
}

/*
 * The parent and child process have their own virtual memory address space, so they both maintain their own 'x's after fork
 */
