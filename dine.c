#include "dine.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Questions for nico

*/

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif

int main(int argc, char *argv[]) {
	int sem_check;
	int thread_check;
	//first init the printing semaphore, pshared is 0 (for threads)
	//value is 1, want to be priting by one thread at a time
	sem_check = sem_init(&status_sem, 0, 1);
	if (sem_check == -1) {
		perror("error on printing semaphore init");
		return 1;
	}

	if (argc == 2) {
		char *end;
		long value = strtol(argv[1], &end, 10);
		if (value < 0 || *end != '\0') {
			fprintf(stderr, "Cycles must be a non-negative \
							 integer\n");
			return 1;
		}
		cycles = (int)value;
	} else if (argc > 2) {
		fprintf(stderr, "usage: ./dine <num cycles (optional)>\n");
		return 1;
	} 

	if (NUM_PHILOSOPHERS < 2) {
		fprintf(stderr, "Need atleast 2 philosophers\n");
		return 1;
	}
	print_line();
	print_label();
	print_line();

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
		// when passing the address of variable it will change 
		// throughout loop
		// array is easiest to create/hold all ids for
		// NUM_PHILOSOPHERS size
		ids[i] = i;
		thread_check = pthread_create(&philosophers[i], NULL, 
						phil_cycle, (void *)(&ids[i]));
		if (thread_check != 0) {
			fprintf(stderr, "fail to create philosopher thread \
						%s\n",strerror(thread_check));
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
			fprintf(stderr, "fail to remove philosopher thread \
						 %s\n",strerror(thread_check));
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
		}
	}
	 
}

void fork_pos_init() {
	int i = 0;
	for (i = 0; i < NUM_FORKS; i++) {
		fork_possession[i] = -1;
		prev_fork_possession[i] = -1;
	}
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
		update_state(id, SAME_STATE, right_fork, id);

		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for even left fork");
			exit(1);
		}
		update_state(id, SAME_STATE, left_fork, id);
	} else {
		sem_check = sem_wait(&forks[left_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd left fork");
			exit(1);
		}
		update_state(id, SAME_STATE, left_fork, id);

		sem_check = sem_wait(&forks[right_fork]);
		if (sem_check == -1) {
			perror("fail to wait for odd right fork");
			exit(1);
		}
		update_state(id, SAME_STATE, right_fork, id);
	}
}

void drop_forks(int id, int left_fork, int right_fork) {
	int sem_check;
	sem_check = sem_post(&forks[left_fork]);
	if (sem_check == -1) {
		perror("failed to post left fork");
	}
	//id doesn't matter here
	update_state(id, SAME_STATE, left_fork, -1);

	sem_check = sem_post(&forks[right_fork]);
	if (sem_check == -1) {
		perror("failed to post right fork");
	}
	update_state(id, SAME_STATE, right_fork, -1);
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
		update_state(id, CHANGE_STATE, NUM_FORKS, NUM_FORKS);
		get_forks(id, left_fork, right_fork);
		update_state(id, EAT_STATE, NUM_FORKS, NUM_FORKS);
		dawdle(); // eating
		update_state(id,CHANGE_STATE, NUM_FORKS, NUM_FORKS);
		drop_forks(id, left_fork,right_fork);
		update_state(id, THINK_STATE, NUM_FORKS, NUM_FORKS);
		dawdle(); // thinking
		

	}
	update_state(id, CHANGE_STATE, NUM_FORKS, NUM_FORKS);

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

//pass SAME_STATE to not update state 
// pass NUM_FORKS for fork to not update possessions
void update_state(int id, int state, int fork, int possession) {
	int sem_check;
	int changed = 0;
	sem_check = sem_wait(&status_sem);
	if (sem_check == -1) {
		perror("fail waiting for status sem");
		exit(1);
	}
	
	// printf("here, id:%d state:%d\n",id, state);
	// printf("states[%d] is %d\n",id, states[id]);
	if (possession == -1) {
		printf("fork %d is being dropped by id %d\n",fork, id);
	}
	if (fork != NUM_FORKS && fork_possession[fork] != possession) {
		// printf("we getting here?\n");
		printf("previously fork %d was owned by %d curr in %d\n",
				 fork,fork_possession[fork], id);
		if (fork_possession[fork] == id || 
				fork_possession[fork] == -1) {
			fork_possession[fork] = possession;
			printf("updating fork pos for id:%d, fork %d \
				now is owned by %d\n", id,fork, 
				fork_possession[fork]);
			if (prev_fork_possession[fork] != fork_possession[fork])
			{
				changed = 1;
			}	
			prev_fork_possession[fork] = fork_possession[fork];
		}
		
	}

	if (state != SAME_STATE && states[id] != state) {
		// printf("we getting here\n");

		states[id] = state;
		if (prev_states[id] != states[id]){
			changed = 1;
		}	
		prev_states[id] = states[id];
	}

	if (changed) {
		printf("id:%d is printing\n",id);
		print_states();
	}

	sem_check = sem_post(&status_sem);
	if (sem_check == -1) {
		perror("fail post for status sem");
		exit(1);
	}

}

void print_states() {
	int i;
	int j;
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
			printf("       ");
		}
	}
	printf("|\n");

}
