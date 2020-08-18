/**
 * Computer Opertaing Systems Homework 3
 * Name				: Alp Eren Gençoğlu
 * Student number	: 150170019
 * Date				: 12.05.2020
 * 
 * compile: 
 * gcc -std=c99 hw3.c -o hw3
 * example run:
 * ./hw3 150 4 2 2 4
 */

#define _GNU_SOURCE		// involve int var inside strcat

#include <stdio.h>

//#include <pthread.h>	// threads
#include <unistd.h>		// fork, sleep func
#include <sys/wait.h>	// wait func
#include <stdlib.h>		// exit func

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

#include <string.h>
#include <stdbool.h>

/*
#define SHMMONEY ftok(strcat(get_current_dir_name(), argv[0]), 1)
#define SEMINIT ftok(strcat(get_current_dir_name(), argv[0]), 2)
#define SEMLOCK ftok(strcat(get_current_dir_name(), argv[0]), 3)
#define SEMTERM ftok(strcat(get_current_dir_name(), argv[0]), 4)
*/

int SHMMONEY;
int SHMINCARR, SHMDECARR;
int SHMINCTIME, SHMRCHDN;

int SEMINIT, SEMLOCK, SEMTERM;
int SEMITF, SEMDTF;
int SEMDIE;

void initKeys(char* argv[]) {
	char cwd[256];
	char* keyString;
	
	//  get current working directory
	getcwd(cwd, 256);
	
	//  form keystring
	keyString = malloc(strlen(cwd) + strlen(argv[0]) + 1);
	strcpy(keyString, cwd);
	//strcat(keyString, argv[0]);
	//printf("%s\n", keyString);	// DEBUG
	
	SHMMONEY = ftok(keyString, 1);
	SEMINIT = ftok(keyString, 2);
	SEMLOCK = ftok(keyString, 3);
	SEMTERM = ftok(keyString, 4);
	SEMITF = ftok(keyString, 5);
	SEMDTF = ftok(keyString, 6);
	SHMINCARR = ftok(keyString, 7);
	SHMDECARR = ftok(keyString, 8);
	SHMINCTIME = ftok(keyString, 9);
	SHMRCHDN = ftok(keyString, 10);
	SEMDIE = ftok(keyString, 11);

	
	// deallocate keystring
	free(keyString);
}



//  increment operation
void sem_signal(int semid, int val) {
	struct sembuf semaphore;
	semaphore.sem_num = 0;
	semaphore.sem_op = val;  //  relative:  add sem_op to value
	semaphore.sem_flg = 0;   
	semop(semid, &semaphore, 1);
}

//  decrement operation
void sem_wait(int semid, int val){
	struct sembuf semaphore;
	semaphore.sem_num = 0;
	semaphore.sem_op = (-1*val);  //  relative:  add sem_op to value
	semaphore.sem_flg = 0;  
	semop(semid, &semaphore, 1);
}

int fib(int n){
	if(n == 0 || n == 1) return 1;
	return fib(n-1) + fib(n-2);
}

int main(int argc, char* argv[]){
	if(argc != 6){
		printf("Error!\n Example Usage: ./yourprogram 150 4 2 2 4\n");
		return -1;	
	}
	initKeys(argv);

	int N, ni, nd, ti, td;

	N = atoi(argv[1]);
	ni = atoi(argv[2]);
	nd = atoi(argv[3]);
	ti = atoi(argv[4]);
	td = atoi(argv[5]);

	int np = ni + nd;	// total number of child processes
	int children[np];
	int incrementers[ni];
	int decrementers[nd];

	/*** semaphore variables ***/
	// semaphore to prevent children to start before 
	// main process is done with initialization.
	int sem_init = semget(SEMINIT, 1, 0700|IPC_CREAT);
	semctl(sem_init, 0, SETVAL, 0);
	// semaphore to create mutual exclusion for money shared memory (money lock)
	int sem_mnylck = semget(SEMLOCK, 1, 0700|IPC_CREAT);
	semctl(sem_mnylck, 0, SETVAL, 0);
	// semaphore to signal termination
	int sem_term = semget(SEMTERM, 1, 0700|IPC_CREAT);
	semctl(sem_term, 0, SETVAL, 0);
	// increasers' turn is finished semaphore
	int sem_itf = semget(SEMITF, 1, 0700|IPC_CREAT);
	semctl(sem_itf, 0, SETVAL, 0);
	// decreasers' turn is finished semaphore
	int sem_dtf = semget(SEMDTF, 1, 0700|IPC_CREAT);
	semctl(sem_dtf, 0, SETVAL, 0);
	// semaphore to prevent children from continuing while master is terminating.
	int sem_die = semget(SEMDIE, 1, 0700|IPC_CREAT);
	semctl(sem_die, 0, SETVAL, 0);	// set to 0, never becomes 1.


/*	These are used for DEBUG purposes
	printf("KEYS: %d %d %d %d %d\n", SEMINIT, SEMLOCK, SEMTERM, SEMITF, SEMDTF);
	printf("IDs: %d %d %d %d %d\n", sem_init, sem_mnylck, sem_term, sem_itf, sem_dtf);
*/

	/*** shared memory variables ***/
	// money box
	int shm_money = shmget(SHMMONEY, sizeof(int), 0700|IPC_CREAT);
	int* money_box;
	// array to see which increasers finished their job
	int shm_incarr = shmget(SHMINCARR, sizeof(bool)*ni, 0700|IPC_CREAT);
	bool* inc_arr;
	// array to see which decreasers finished their job
	int shm_decarr = shmget(SHMDECARR, sizeof(bool)*nd, 0700|IPC_CREAT);
	bool* dec_arr;
	// keep track of whose turn is it
	int shm_inctime = shmget(SHMINCTIME, sizeof(bool), 0700|IPC_CREAT);
	bool* increase_time;
	// keep track of if N is reached or not
	int shm_reachedN = shmget(SHMRCHDN, sizeof(bool), 0700|IPC_CREAT);
	bool* reachedN;

/*	These are used for DEBUG purposes
	printf("KEYS: %d %d %d %d %d\n", SHMMONEY, SHMINCARR, SHMDECARR, SHMINCTIME, SHMRCHDN);
	printf("IDs: %d %d %d %d %d\n", shm_money, shm_incarr, shm_decarr, shm_inctime, shm_reachedN);
*/

	/*** create child processes ***/
	int f = 1, i;
	for (i = 0; i < ni+nd; ++i){
		if(f > 0)
			f = fork();
		if(f == -1){
			printf("fork error\n");
			return -2;
		}

		// child is created, check f's value
		if(f == 0)	// child
			break;
		else {		// main
			children[i] = f;	// save pids of children
			if(i < ni)
				incrementers[i] = f;
			else
				decrementers[i-ni] = f;
		}
	}

	// main process operations
	if(f > 0){
		// create a money box, a shared memory with size of 
		// one integer, which is for storing money.
		money_box = (int*) shmat(shm_money, 0, 0);	// money_box pointer is attached
		*money_box = 0;		// initialize the money amount as 0.
		printf("Master process: current money is %d\n", *money_box);
		shmdt(money_box);	// detach money_box pointer

		// reset shared arrays
		inc_arr = (bool*) shmat(shm_incarr, 0, 0);
		for(int b = 0; b < ni; b++) inc_arr[b] = false;
		shmdt(inc_arr);
		dec_arr = (bool*) shmat(shm_decarr, 0, 0);
		for(int b = 0; b < ni; b++) dec_arr[b] = false;
		shmdt(dec_arr);

		//set increase_time = true for beginning (initially only increasers work)
		increase_time = (bool*) shmat(shm_inctime, 0, 0);
		*increase_time = true;
		shmdt(increase_time);

		// initialize reachedN as false
		reachedN = (bool*) shmat(shm_reachedN, 0, 0);
		*reachedN = false;
		shmdt(reachedN);



		// initialize some semaphore values
		semctl(sem_term, 0, SETVAL, 0);	// set termination signal to 0
		sem_signal(sem_init, np);	// allow np child processes to begin
		sem_signal(sem_mnylck, 1);	// remove lock of shm for 1 process

		// wait for the termination signal
		sem_wait(sem_term, 1);
		printf("Master Process: Killing all children and terminating the program\n");
		for (int p = 0; p < np; ++p) {
			kill(children[p], SIGKILL);
		}

		//while( wait(NULL) > 0 ) ; 	// wait all child processes
	}

	// child process operations
	if(f == 0){
		sem_wait(sem_init, 1);	// wait until main process allows

		money_box = (int*) shmat(shm_money, 0, 0);	// money_box pointer is attached
		int temp_money;
		int amount;	// increase / decrease amount for money_box

		increase_time = (bool*) shmat(shm_inctime, 0, 0);	// dec or inc turn?
		reachedN = (bool*) shmat(shm_reachedN, 0, 0);
		
		int iturn = 1;	// increasers' turn number
		int dturn = 1;	// decreasers' turn number
		int fib_index = 0;	// index for fibonecci series, used by decreasers. 
							// each decreaser has its own fib_index
		while(true){
			if(i < ni && *increase_time){	// incrementer process
				/*
				 *
				 */
				inc_arr = (bool*) shmat(shm_incarr, 0, 0);

				int i_run_until = iturn + ti;
				for (iturn; iturn < i_run_until; ++iturn){
					// critical section begin
					sem_wait(sem_mnylck, 1);
					if(i % 2 == 0) amount = 10;
					else amount = 15;
					temp_money = *money_box;
					usleep(100000);
					temp_money += amount;
					*money_box = temp_money;
					printf("Increaser Process %d: Current money is %d", i, *money_box);
					sem_signal(sem_mnylck, 1);
					// critical section end
					/*
					* after current process finishes its job, check if
					* other processes are done too. if so, it means one turn 
					* is complete so continue, otherwise wait for them.
					*/
					*(inc_arr+i) = true;
					int b;
					for(b = 0; b < ni; b++){
						if(!inc_arr[b]) break;
					}
					if(b == ni){	// all elements in array are true
						printf(", Increaser Processes finished their turn %d\n", iturn);
						if(*money_box >= N){
							//printf("REACHED N\n");
							*reachedN = true;
						}
						for(b = 0; b < ni; b++) inc_arr[b] = false;	// reset the array.
						if(*reachedN && iturn == i_run_until-1){
							printf("\n");
							*increase_time = false;	// it is now decreasers' turn
						}
						sem_signal(sem_itf, ni);	// signal all ni increasers are done.
					}
					else{
						printf("\n");
					}
					sem_wait(sem_itf, 1);
				}
			}

			if (i >= ni && !(*increase_time) ){	// decrementer process
				/*
				 * waits until the "money >= N" signal
				 * of a semaphore, then starts decrementing
				 * if this is its turn.
				 */
				dec_arr = (bool*) shmat(shm_decarr, 0, 0);
				int d_run_until = dturn + td;
				for (dturn; dturn < d_run_until; ++dturn){
					
					// critical section begin
					sem_wait(sem_mnylck, 1);
					bool cond1 = (*money_box % 2 == 0 && (i-ni) % 2 == 0);
					bool cond2 = (*money_box % 2 == 1 && (i-ni) % 2 == 1);
					if(cond1 || cond2)
					{
						// even numbered decreasers decrease if money is even,
						// odd numbered decreasers decrease if money is odd.
						amount = fib(fib_index);
						temp_money = *money_box;
						usleep(100000);
						if(amount >= temp_money){
							printf("Decreaser Process %d: Current money is less than %d, signaling master to finish (%dth fibonacci number for decreaser %d)\n", i-ni, amount, ++fib_index, i-ni);
							sem_signal(sem_term, 1);
							sem_wait(sem_die, 1);	// wait to die.
						}
						temp_money -= amount;
						*money_box = temp_money;
						printf("Decreaser Process %d: Current money is %d (%dth fibonacci number for decreaser %d)",i-ni,*money_box,++fib_index,i-ni);
					}
					sem_signal(sem_mnylck, 1);
					// critical section end

					*(dec_arr+(i-ni)) = true;
					int b;
					for(b = 0; b < nd; b++){
						if(!dec_arr[b]) break;
					}
					if(b == nd){	// all elements in array are true
						printf(", Decreaser Processes finished their turn %d\n", dturn);
						for(b = 0; b < nd; b++) dec_arr[b] = false;	// reset the array.
						if(dturn == d_run_until-1){
							printf("\n");
							*increase_time = true;	// it is now increasers' turn
						}
						sem_signal(sem_dtf, nd);	// signal all nd decreasers are done.
					}
					else {
						if(cond1 || cond2)
							printf("\n");
					}
					sem_wait(sem_dtf, 1);
				}
			}

		}
	}

	return 0;
}