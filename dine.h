#include <semaphore.h>
#include <pthread.h>

#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 10
#endif

#define NUM_FORKS NUM_PHILOSOPHERS
#define COL_WIDTH_MIN 8 // column width
#define MIN_PAD 4 //space pads needed for printing
#define EVEN_MULTIPLE 2 // Used to see if even
#define LAST_ASCII 127 // DEL ascii
#define START_ASCII 65 // A ascii
#define MAX_PHIL LAST_ASCII - START_ASCII
#define AVAIL -1

#define CHANGE_STATE 0
#define EAT_STATE 1
#define THINK_STATE 2
#define SAME_STATE 3


//started off with the arrays, but grew fast
// if done in the future would potentionally do struct
//however would still want a fork possession and id array
int cycles = 1;
sem_t status_sem;
sem_t forks[NUM_FORKS];
pthread_t philosophers[NUM_PHILOSOPHERS];
int ids[NUM_PHILOSOPHERS];
int states[NUM_PHILOSOPHERS];
int fork_possession[NUM_FORKS];
int prev_states[NUM_PHILOSOPHERS];
int prev_fork_possession[NUM_FORKS];


void init_forks();
void get_forks(int id, int left_fork, int right_fork);
void drop_forks(int id, int left_fork, int right_fork);
void dawdle();
void * phil_cycle(void * phil_id);
int remove_philosophers();
int create_philosophers();
void destroy_forks();
void fork_pos_init();
void update_state(int id, int state, int fork, int possession);
void random_time_test();

//------------ PRINT FUNCTIONS
void print_line();
void print_label();
void print_states();

