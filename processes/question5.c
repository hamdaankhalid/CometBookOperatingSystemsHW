/*
 * Write a program that uses wait for child proc to finish what does wait return? what happens if we use wait in child proc?
 */

#include<unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main() {
	int pid = fork();

	if (pid < 0) {
		return -1;
	} else if (pid == 0) {
		printf("Child Proc \n");
	} else {
		printf("In Parent Proc we know pid of child proc is: %d \n", pid);
		int resp = wait(&pid);
		printf("Parent Proc Wait: %d \n", resp);
	}
}

// Calling wait in child proc throws an error! wait returns the address of the waited proccess
