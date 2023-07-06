/*
 * Write a program that calls fork and then some form of exec to run bin/ls. Try different variants for exec.
 * Why are there so many?
 * */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	int pid = fork();
	
	if (pid < 0) {
		printf("fork you \n");
		return -1;
	} else if (pid == 0) {
		// child proc
		// call exec
		char *args[] = {"bin/ls", "-l", NULL};  // Example command: ls -l
		execvp("ls", args);
	} else {
		wait(&pid);
		printf("Parent proc has finished \n");
	}
}
