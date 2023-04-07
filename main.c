#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;  // save time for begin, end
	gettimeofday(&begin_t, NULL);  // save program begin time

	ssu_score(argc, argv);  // driver code

	gettimeofday(&end_t, NULL);  // save program end time
	ssu_runtime(&begin_t, &end_t);  // print program running time

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	end_t->tv_sec -= begin_t->tv_sec;  // calculate sec diff

	if(end_t->tv_usec < begin_t->tv_usec){  // if ms < 0
		end_t->tv_sec--;  // - 1sec (1000ms)
		end_t->tv_usec += SECOND_TO_MICRO;  // + 1000ms (1 sec)
	}

	end_t->tv_usec -= begin_t->tv_usec;  // calculate usec diff
	// print running time
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec); 
}
