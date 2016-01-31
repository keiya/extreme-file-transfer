/* https://gist.github.com/jbenet/1087739 */
/* 
author: jbenet
os x, compile with: gcc -o testo test.c 
linux, compile with: gcc -o testo test.c -lrt
*/

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#define BILLION 1000000000L

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


void current_utc_time(struct timespec *ts) {

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif

}

/* https://www.cs.rutgers.edu/~pxk/416/notes/c-tutorials/gettime.html */
long long unsigned int nanodiff(struct timespec *start, struct timespec *end)
{
	long long unsigned int diff;
	diff = BILLION * (end->tv_sec - start->tv_sec) + end->tv_nsec - start->tv_nsec;
	return diff;
}


/*
int main(int argc, char **argv) {

  struct timespec ts;
  current_utc_time(&ts);

  printf("s:  %lu\n", ts.tv_sec);
  printf("ns: %lu\n", ts.tv_nsec);
  return 0;

}
*/
