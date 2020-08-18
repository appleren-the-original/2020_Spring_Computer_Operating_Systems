#include <stdio.h>
#include <unistd.h>		//  fork func
#include <sys/wait.h>	//  wait func
#include <stdlib.h>		//  exit func


int main(int argc, char const *argv[]) {
	int c = 0;
	int child = fork();
	c++;
	if(child == 0) {
		child = fork();
		c += 2;
		if(child) c = c*3;
	}
	else {
		c += 4;
		fork();
	}
	
	return 0;
}