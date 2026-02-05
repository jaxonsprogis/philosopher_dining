#include "dine.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif

int main(int argc, char *argv[]) {
	int sem_check;
	int thread_check;
	//first init the status semaphore, pshared is 0 (for threads)
	//value is 1, want to be priting by one thread at a time
	sem_check = sem_init(&status_sem, 0, 1);
	if (sem_check == -1) {
		perror("error on printing semaphore init");
		return 1;
	}
	// argument checking, only want two arguments
	// expect two, 1 for executable and 1 for optional cycle amt.
	if (argc == 2) {
		// if right amount, check argument for integer
		// set cycles
		char *end;
		long value = strtol(argv[1], &end, 10);
		if (errno == ERANGE) {
			fprintf(stderr,"Value out of range\n");
			return 1;
		}
		if (value < 0 || *end != '\0') {
			fprintf(stderr, "Cycles must be non-negative int:\n");
			return 1;
		}
		cycles = (int)value;
	} else if (argc > 2) {
		//gave too many arguments, not needed 
		fprintf(stderr, "usage: ./dine <num cycles (optional)>\n");
		return 1;
	} 
	//check if the current amount of philosophers is in correct range
	if (NUM_PHILOSOPHERS < 2 || NUM_PHILOSOPHERS > MAX_PHIL) {
		fprintf(stderr, "Need between 2 and 62 philosophers\n");
		return 1;
	} 
	//prints the top label
	print_line();
	print_label();
	print_line();

	//initializes the fork semaphores and places them in array
	init_forks();
	//inits all fork possession in array to -1 (AVAILABLE)
	fork_pos_init();
	random_time_test(); // sets srandom for randomization dawdle

	//creates the philosopher threads
	thread_check = create_philosophers();
	if (thread_check != 0) {
		return 1;
	}
	// removes/waits on the philosopher threads to complete
	thread_check = remove_philosophers();
	if (thread_check != 0) {
		return 1;
	}

	//destroy the fork semaphores once everyone done
	destroy_forks();
	//print the last line (border) in the table
	print_line();
	return 0;
}

//retrieves time of day and sets seed for srandom
//randomizes our dawdle function
void random_time_test() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srandom(tv.tv_sec);
}

//creates all the philosopher threads, passes their id argument
// and links their function to phil_cycle
int create_philosophers() {
	int i = 0;
	int thread_check = 0;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		// each id needs to be saved in a variable of some sort,
		// when passing the address of variable it will change 
		// throughout loop
		// array is easiest to create/hold all ids for
		// NUM_PHILOSOPHERS size
		ids[i] = i;
		//create the thread
		thread_check = pthread_create(&philosophers[i], NULL, 
						phil_cycle, (void *)(&ids[i]));
		//check if thread creation had trouble
		if (thread_check != 0) {
			fprintf(stderr, "fail to create philosopher thread \
						%s\n",strerror(thread_check));
			return 1;
		}
	}
	return 0;
}

//waits for all the philosopher threads
int remove_philosophers() {
	int i = 0;
	int thread_check = 0;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		// for every philosopher thread wait for each
		thread_check = pthread_join(philosophers[i], NULL);
		if (thread_check != 0) {
			fprintf(stderr, "fail to remove philosopher thread \
						 %s\n",strerror(thread_check));
			return 1;
		}
	}
	return 0;
}

//init all forks, create all semaphores
// store the forks within an array
void init_forks() {
	int i = 0;
	int sem_check;
	for (i = 0; i < NUM_FORKS; i++) {
		//init semaphore, value is 1, pshared is 0
		sem_check = sem_init(&forks[i], 0, 1);
		if (sem_check == -1) {
			perror("error on fork semaphore init");
			exit(1);
		}
	}
}
// destroy for all forks within fork array
void destroy_forks() {
	int i = 0;
	int sem_check;
	for (i = 0; i < NUM_FORKS; i++) {
		sem_check = sem_destroy(&forks[i]);
		if (sem_check == -1) {
			perror("error on fork semaphore destroy");
		}
	}
	 
}

//initialize the fork possession array to have all -1
// -1 is not an attainable id therefore it is available
void fork_pos_init() {
	int i = 0;
	for (i = 0; i < NUM_FORKS; i++) {
		fork_possession[i] = AVAIL;
		prev_fork_possession[i] = AVAIL;
	}
}

//a function that a philosopher will call to pick up forks
//based on their id, decides what fork to pick up
void get_forks(int id, int left_fork, int right_fork) {
	// if id is even then pick up right fork first 
	int sem_check;
	if (id % EVEN_MULTIPLE == 0) {
		//pick up right fork or block for it 
		sem_check = sem_wait(&forks[right_fork]);
		if (sem_check == -1) {
			perror("fail to wait for even right fork");
			exit(1);
		}
		//update the state (Print state change)
		update_state(id, SAME_STATE, right_fork, id);

		//pick up left fork or block for it 

		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for even left fork");
			exit(1);
		}
		//update the state (Print state change)

		update_state(id, SAME_STATE, left_fork, id);
	} else {
		//if we are here then id is odd
		//pick up left fork or block for it 
		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd left fork");
			exit(1);
		}
		//update the state (Print state change)
		update_state(id, SAME_STATE, left_fork, id);

		//pick up right fork or block for it 
		sem_check = sem_wait(&forks[right_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd right fork");
			exit(1);
		}
		//update the state (Print state change)
		update_state(id, SAME_STATE, right_fork, id);
	}
}
//a function that a philosopher will call to drop forks
//no ordering to dropping the forks, but left is dropped first
void drop_forks(int id, int left_fork, int right_fork) {
	int sem_check;

	//want to update the state first because there is a chance
	// we drop the fork before we can print status, leading
	// to a double print change
	// wont cause harm because semaphore still intact 
	update_state(id, SAME_STATE, left_fork, AVAIL);

	//drop left fork
	sem_check = sem_post(&forks[left_fork]);
	if (sem_check == -1) {
		perror("failed to post left fork");
	}
	// print change
	update_state(id, SAME_STATE, right_fork, AVAIL);
	//drop right fork
	sem_check = sem_post(&forks[right_fork]);
	if (sem_check == -1) {
		perror("failed to post right fork");
	}
	
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
//main philosopher function that pick up forks
// eats, changes, thinks
void * phil_cycle(void * phil_id) {
	int id = *(int *)phil_id;
	//find left and right fork off of id
	int left_fork = id;
	int right_fork = id + 1;
	if (right_fork >= NUM_FORKS) {
		right_fork = 0;
	}
	int i;
	// for every cycle eat think repeat
	for (i = 0; i < cycles; i++) {
		//start in change state
		update_state(id, CHANGE_STATE, NUM_FORKS, NUM_FORKS);
		//pick up forks
		get_forks(id, left_fork, right_fork);
		//change to eat state
		update_state(id, EAT_STATE, NUM_FORKS, NUM_FORKS);
		dawdle(); // eating
		//change to change state
		update_state(id,CHANGE_STATE, NUM_FORKS, NUM_FORKS);
		//drop forks
		drop_forks(id, left_fork,right_fork);
		//change to think state
		update_state(id, THINK_STATE, NUM_FORKS, NUM_FORKS);
		dawdle(); // thinking
		

	}
	//end in change state
	update_state(id, CHANGE_STATE, NUM_FORKS, NUM_FORKS);
	return NULL;

}
// doesnt need semaphore, only used in main thread
// prints the solid line used for top and bottom border 
void print_line() {
	int i;
	int j;
	//find column size, based off minimum and philosophers needed
	int column_size = COL_WIDTH_MIN + NUM_PHILOSOPHERS;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		//print divider 
		printf("|");
		for (j = 0; j < column_size; j++) {
			printf("=");
		}
	}
	printf("|\n");
}
//called once in main, no semaphore needed
// prints the top part, only the label line
void print_label() {
	//starts at character A
	char label = 'A';
	int i;
	int j;
	// finds column size and the amount to pad to keep centered
	int column_size = COL_WIDTH_MIN + NUM_PHILOSOPHERS;
	//the minimum pad is found with only 2 philosophers
	int pad_amt = MIN_PAD + NUM_PHILOSOPHERS/2;
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		printf("|");
		for (j = 0; j < column_size; j++) {
			if (j == pad_amt) {
				//if at pad_amt we are at center
				//print the character
				printf("%c",label);
				//get next character
				label++;
			} else {
				printf(" ");
			}
		}
	}
	printf("|\n");
}
//function that uses semaphore for printing each status line
// either updates the fork possession array or state array
//pass SAME_STATE for state to not update state 
// pass NUM_FORKS for fork to not update possessions
void update_state(int id, int state, int fork, int possession) {
	int sem_check;
	int changed = 0;
	//waits for the status update semaphore
	sem_check = sem_wait(&status_sem);
	if (sem_check == -1) {
		perror("fail waiting for status sem");
		exit(1);
	}
	//checks if fork == NUM_FORKS or if already possessed
	// helps remove double print status statements
	if (fork != NUM_FORKS && fork_possession[fork] != possession) {
		//update forkpossession to the id (possession)
			fork_possession[fork] = possession;
			if (prev_fork_possession[fork] != 
						fork_possession[fork]) {
				//if it wasn't changed from previous 
				// then nothing changed
				changed = 1;
			}	
			prev_fork_possession[fork] = fork_possession[fork];
		
	}
	//checks if in same_state and we don't need to update state
	if (state != SAME_STATE && states[id] != state) {
		//update the id current state to state
		states[id] = state;
		if (prev_states[id] != states[id]){
			//if it wasn't changed from previous 
			//then nothing changed
			changed = 1;
		}	
		prev_states[id] = states[id];
	}

	if (changed) {
		//does the actual printing
		print_states();
	}
	//gives the semaphore back for another thread
	sem_check = sem_post(&status_sem);
	if (sem_check == -1) {
		perror("fail post for status sem");
		exit(1);
	}

}

//does the actual printing of all the status lines
void print_states() {
	int i;
	int j;
	//starts the fork values at 0
	char start_char = '0';
	//go through every philosopher and will check 
	// possession and state
	for (i = 0; i < NUM_PHILOSOPHERS; i++) {
		printf("| ");
		start_char = '0';
		for (j = 0; j < NUM_FORKS; j++) {
			if (fork_possession[j] == i) {
				//if current fork is owned by current 
				//philosopher, print that
				printf("%c",start_char);
			} else {
				printf("-");
			}
			//go to next fork value
			start_char++;
		}
		//check the states for this philosopher
		if (states[i] == EAT_STATE) {
			printf(" Eat   ");
		} else if (states[i] == THINK_STATE) {
			printf(" Think ");
		} else {
			printf("       ");
		}
	}
	printf("|\n");
}
