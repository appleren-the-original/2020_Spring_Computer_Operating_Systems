/**
 * Computer Opertaing Systems Homework 2
 * Name				: Alp Eren Gençoğlu
 * Student number	: 150170019
 * Date				: 08.04.2020
 * 
 * compile: 
 * gcc -std=c99 hw2.c -o hw2c -pthread
 * example run:
 * ./hw2c 101 200 2 2
 */

#define _GNU_SOURCE		// involve int var inside strcat

#include <stdio.h>
#include <pthread.h>	// threads
#include <unistd.h>		// fork, sleep func
#include <sys/wait.h>	// wait func
#include <stdlib.h>		// exit func
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

#define shmkey(x) ftok("hw2.c", x)

typedef struct {
	int p, t, start, end;
	int* shm_ptr;
} Args;

bool isPrime(int n){
	if (n <= 1) return false;
	if (n == 2 || n == 3) return true;
	if (n%2 == 0 || n%3 == 0) return false; 

	for (int i=5; i*i<=n; i=i+6)
		if (n%i == 0 || n%(i+2) == 0) 
			return false; 

	return true;
}

void* intervalPrimes(void* _args){
	Args* args = (Args*) _args;
	int p = args->p, t = args->t, start = args->start, end = args->end;
	int* shm_ptr = args->shm_ptr;
	printf("Thread %d.%d: searching in %d-%d \n", p+1, t+1, start, end);

	for(int i=start; i<=end; i++){
		if(isPrime(i)) {
			*shm_ptr = i; shm_ptr++;
		}
	}
	*shm_ptr = -1;

	long exit_status = 10;		// dummy value
	pthread_exit((void*) exit_status);
}

int main(int argc, char const *argv[]) {
	if(argc != 5){
		printf("Error!\nUsage: ./yourprogram interval_min interval_max np nt\n");
		return -1;	
	}


	int min, max, np, nt;

	min = atoi(argv[1]);
	max = atoi(argv[2]);
	np = atoi(argv[3]);
	nt = atoi(argv[4]);

	int f = 1;

	int main_process = getpid();
	printf("Master (pid: %d): Started.\n", main_process);
	
	int range = max - min;
	int step = range / (np*nt);
	int p;
	for(p=0; p<np; p++){
		if(f > 0){			// main
			f = fork();
		}

		if(f==0){		// child process
			printf("Slave %d (pid: %d): Started. Interval %d-%d\n", p+1, getpid(), min, ((min+step*nt+nt-1 <= max) ? min+step*nt+nt-1 : max));
			
			// thread operations
			pthread_t threads[nt];
			pthread_attr_t attr;
			void* status;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

			// shared memory operations
			int slave_process_range = range / np;
			int shm_size = sizeof(int) * (slave_process_range + 1);
			int shm_id = shmget(shmkey(p), shm_size, 0700|IPC_CREAT);
			int* shm_ptr = (int*) shmat(shm_id, 0, 0);
			
			for(int t=0; t<nt; t++, min+=step+1){
				int start = min;
				int end = (min+step <= max) ? min+step : max ;
				Args* arg = (Args*) malloc(sizeof(Args));
				arg->p = p; arg->t = t; arg->start = start; arg->end = end;
				arg->shm_ptr = shm_ptr + (t*step);
				if(pthread_create(&threads[t], NULL, intervalPrimes, (void*)arg))
					printf("THREAD CREATION ERROR\n");
			}
			for(int t=0; t<nt; t++, min+=step+1){
				pthread_join(threads[t], &status);	// status will have the value determined in the thread routine
			}

			break;
		}
		//wait(NULL);

		min += step*nt + nt;
	}

	if(f == 0){
		printf("Slave %d: Done.\n", p+1);
	}
	else if (f > 0) { 
		//wait(NULL);
		while (wait(NULL) > 0) ;
		int shm_ids[np];
		for(int i=0; i<np; i++){
			int slave_process_range = range / np;
			int shm_size = sizeof(int) * (slave_process_range + 1);
			shm_ids[i] = shmget(shmkey(i), shm_size, 0);
		}
		printf("Master: Done. Prime numbers are: ");
		for(int i=0; i<np; i++){
			for(int t=0; t<nt; t++){
				int* shm_ptr = (int*) shmat(shm_ids[i], 0, 0);
				shm_ptr += t*step;
				while(*shm_ptr != -1){
					printf("%d, ", *shm_ptr);
					shm_ptr++;
				}
				//free(shm_ptr);
			}
			shmctl(shm_ids[i], IPC_RMID, NULL); 
		}
		printf("\n");
	}

	pthread_exit(NULL);

	return 0;
}