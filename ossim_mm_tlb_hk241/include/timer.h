#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdint.h>

struct timer_id_t {
	int done;
	int fsh;
	int cpu_wait;
	int time_wait;
	pthread_cond_t event_cond;
	pthread_mutex_t event_lock;
	pthread_cond_t timer_cond;
	pthread_mutex_t timer_lock;
};

void start_timer();

void stop_timer();

struct timer_id_t * attach_event();

void detach_event(struct timer_id_t * event);

void next_slot(struct timer_id_t* timer_id);

uint64_t current_time();

void wait_time();
void wait_cpu();
void unlock_cpu();
#endif
