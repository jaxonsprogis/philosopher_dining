#include "dine.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Questions for nico
 1. should we check argument to see if only an integer, what if set to 0?
 2. if any of the semaphore inits fail should we return?
 3. dropping the forks doesnt have a required order right

*/

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif

int main(int argc, char *argv[]) {
	print_line();
	print_label();
	print_line();

	int sem_check;
	int thread_check;
	//first init the printing semaphore, pshared is 0 (for threads)
	//value is 1, want to be priting by one thread at a time
	sem_check = sem_init(&print_sem, 0, 1);
	if (sem_check == -1) {
		perror("error on printing semaphore init");
		return 1;
	}

	if (argc == 2) {
		cycles = atoi(argv[1]);
	} else if (argc > 2) {
		fprintf(stderr, "usage: ./dine <num cycles (optional)>\n");
		return 1;
	} 

	if (NUM_PHILOSOPHERS < 2) {
		fprintf(stderr, "Need atleast 2 philosophers\n");
		return 1;
	}

	init_forks();
	fork_pos_init();
	thread_check = create_philosophers();
	if (thread_check != 0) {
		return 1;
	}
	thread_check = remove_philosophers();
	if (thread_check != 0) {
		return 1;
	}
	destroy_forks();
	print_line();
	return 0;
}

int create_philosophers() {
	int i = 0;
	int thread_check = 0;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		// each id needs to be saved in a variable of some sort,
		// when passing the address of variable it will change throughout loop
		// array is easiest to create/hold all ids for NUM_PHILOSOPHERS size
		ids[i] = i;
		thread_check = pthread_create(&philosophers[i], NULL, phil_cycle, (void *)(&ids[i]));
		if (thread_check != 0) {
			fprintf(stderr, "fail to create philosopher thread %s\n",strerror(thread_check));
			return 1;
		}
	}
	return 0;
}

int remove_philosophers() {
	int i = 0;
	int thread_check = 0;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		thread_check = pthread_join(philosophers[i], NULL);
		if (thread_check != 0) {
			fprintf(stderr, "fail to remove philosopher thread %s\n",strerror(thread_check));
			return 1;
		}
	}
	return 0;
}

void init_forks() {
	int i = 0;
	int sem_check;
	for (i = 0; i < NUM_FORKS; i++) {
		sem_check = sem_init(&forks[i], 0, 1);
		if (sem_check == -1) {
			perror("error on fork semaphore init");
			exit(1);
		}
	}
}

void destroy_forks() {
	int i = 0;
	int sem_check;
	for (i = 0; i < NUM_FORKS; i++) {
		sem_check = sem_destroy(&forks[i]);
		if (sem_check == -1) {
			perror("error on fork semaphore destroy");
			exit(1);
		}
	}
	 
}

void fork_pos_init() {
	int i = 0;
	for (i = 0; i < NUM_FORKS; i++) {
		fork_possession[i] = NUM_FORKS;
	}
	posessions_init = 1;
}
void get_forks(int id, int left_fork, int right_fork) {
	// if id is even then pick up right first 
	int sem_check;
	if (id % EVEN_MULTIPLE == 0) {
		sem_check = sem_wait(&forks[right_fork]);
		if (sem_check == -1) {
			perror("fail to wait for even right fork");
			exit(1);
		}
		fork_possession[right_fork] = id;
		print_states();

		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for even left fork");
			exit(1);
		}
		fork_possession[left_fork] = id;
		print_states();
	} else {
		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd left fork");
			exit(1);
		}
		
		fork_possession[left_fork] = id;
		print_states();

		sem_check = sem_wait(&forks[right_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd right fork");
			exit(1);
		}
		fork_possession[right_fork] = id;
		print_states();
	}
}

void drop_forks(int left_fork, int right_fork) {
	int sem_check;
	sem_check = sem_post(&forks[left_fork]);
	if (sem_check == -1) {
		perror("failed to post left fork");
	}
	fork_possession[left_fork] = NUM_FORKS; // clear fork possesion (high num)
	print_states();
	sem_check = sem_post(&forks[right_fork]);
	if (sem_check == -1) {
		perror("failed to post right fork");
	}
	fork_possession[right_fork] = NUM_FORKS; // clear fork possesion (high num)
	print_states();
}

void dawdle() {
/*
* sleep for a random amount of time between 0 and DAWDLEFACTOR
* milliseconds. This routine is somewhat unreliable, since it
* doesn’t take into account the possiblity that the nanosleep
* could be interrupted for some legitimate reason.
*/
	struct timespec tv;
	int msec = (int)((((double)random()) / RAND_MAX) * DAWDLEFACTOR);
	tv.tv_sec = 0;
	tv.tv_nsec = 1000000 * msec;
	if (-1 == nanosleep(&tv,NULL) ) {
		perror("nanosleep");
	}
}

void * phil_cycle(void * phil_id) {
	int id = *(int *)phil_id;
	int left_fork = id;
	int right_fork = id + 1;
	if (right_fork >= NUM_FORKS) {
		right_fork = 0;
	}
	int i;

	for (i = 0; i < cycles; i++) {
		states[id] = CHANGE_STATE;
		print_states();
		get_forks(id, left_fork, right_fork);
		states[id] = EAT_STATE;
		print_states();
		dawdle(); // eating
		states[id] = CHANGE_STATE;
		print_states();
		drop_forks(left_fork,right_fork);
		states[id] = THINK_STATE;
		print_states();
		dawdle(); // thinking
		
		states[id] = CHANGE_STATE;
		print_states();


	}
	return NULL;

}
// doesnt need semaphore, only used in main thread
void print_line() {
	int i;
	int j;
	int column_size = COL_WIDTH_MIN + NUM_PHILOSOPHERS;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		printf("|");
		for (j = 0; j < column_size; j++) {
			printf("=");
		}
	}
	printf("|\n");
}

void print_label() {
	char label = 'A';
	int i;
	int j;
	int column_size = COL_WIDTH_MIN + NUM_PHILOSOPHERS;
	int pad_amt = MIN_PAD + NUM_PHILOSOPHERS/2;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		printf("|");
		for (j = 0; j < column_size; j++) {
			if (j == pad_amt) {
				printf("%c",label);
				label++;
			} else {
				printf(" ");
			}
		}
	}
	printf("|\n");
}

void print_states() {
	int sem_check;
	int i;
	int j;
	sem_check = sem_wait(&print_sem);
	if (sem_check == -1) {
		perror("fail waiting for print sem");
		exit(1);
	}
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		printf("| ");
		for (j = 0; j < NUM_FORKS; j++) {
			if (fork_possession[j] == i) {
				printf("%d",j);
			} else {
				printf("-");
			}
		}
		if (states[i] == EAT_STATE) {
			printf(" Eat   ");
		} else if (states[i] == THINK_STATE) {
			printf(" Think ");
		} else {
			printf("    1  ");
		}
	}
	printf("|\n");
	sem_check = sem_post(&print_sem);
	if (sem_check == -1) {
		perror("fail post for print sem");
		exit(1);
	}
}
