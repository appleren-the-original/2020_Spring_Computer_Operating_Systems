/*
 * @Author: Alp Eren Gençoğlu
 * Computer Operating Systems Homework 1
 * 07.03.2020
 */

#include <stdio.h>
#include <unistd.h>		//  fork, sleep func
#include <sys/wait.h>	//  wait func
#include <stdlib.h>		//  exit func

/*
TEST
*/

int tree[13][4];

void printNode(int* arr, int size, int level){
	//for(int i=0; i<(level-1)*4; i++)
	//	printf(" ");
	printf("<%d, [", getpid());
	for(int i=0; i<size; i++){
		printf("%d", *(arr+i));
		if(i != size-1) printf(", ");
	}
	printf("], %d>\n", level);
	
}

/*void insertNode(int pid, int p_pid){
	int i = 0, j = 0;
	while(tree[i][0] != 0) i++;
	tree[i][0] = pid;
	
	if(p_pid == 0) return;
	i = 0;
	while(tree[i][0] != p_pid) i++;
	while(tree[i][j] != 0) j++;
	tree[i][j] = pid;
	
	return;
}*/

int main(int argc, char const *argv[]) {
	
	int main_pid = getpid();
	
	int level = 1;
	int f1 = 1, f2 = 1, f3 = 1;
	int i,j,k;
	int level1c[3], level2c[3][2], level3c[3][1];
	
	
	//printNode(level);
	
	for(i=0; i<3; i++){
		if(f1 > 0){			// main
			f1 = fork();
			level1c[i] = f1;
		}
		
		if(f1 == 0){			// level 1 child
			//level1c[i] = getpid();
			level++;
			
			for(j=0; j<2; j++){
				if(f2 > 0){		// level 1 child
					f2 = fork();
					level2c[i][j] = f2;
				}
				if(f2 == 0){		// level 2 child
					//level2c[i][j] = getpid();
					level++;
					
					if(j){			// to create only one child
						f3 = fork();
						level3c[i][0] = f3;
					}
					if(f3 == 0){		// level 3 child
						//level3c[i][0] = getpid();
						level++;
						printNode(level1c, 0, level);
						
					} else{
						wait(NULL);
						if(j) printNode(level3c[i], 1, level);
						else printNode(level3c[i], 0, level);
						break;
					}
				}
				else{
					wait(NULL);
					if(j == 1) printNode(level2c[i], 2, level);
				}
			}
			break;
		}	
		else{
			wait(NULL);
			if(i == 2) printNode(level1c, 3, level);
		}
		
		
	}
	

	return 0;
}