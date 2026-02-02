#include <semaphore.h>
#include <pthread.h>

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 10
#endif

#define NUM_FORKS NUM_PHILOSOPHERS
#define COL_WIDTH_MIN 8
#define MIN_PAD 4
#define EVEN_MULTIPLE 2

#define EAT_STATE 1
#define THINK_STATE 2
#define CHANGE_STATE 3

sem_t print_sem;
int cycles = 1;
int posessions_init = 0;

sem_t forks[NUM_FORKS];
pthread_t philosophers[NUM_PHILOSOPHERS];
int ids[NUM_PHILOSOPHERS];
int states[NUM_PHILOSOPHERS];
int fork_possession[NUM_FORKS];


void init_forks();
void get_forks(int id, int left_fork, int right_fork);
void drop_forks(int left_fork, int right_fork);
void dawdle();
void * phil_cycle(void * phil_id);
int remove_philosophers();
int create_philosophers();
void destroy_forks();
void fork_pos_init();

//------------
void print_line();
void print_label();
void print_states();

