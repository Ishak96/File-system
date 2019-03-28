#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

#include <devutils.h>

/**
 * @brief utility function to terminate program
 */
void die(const char* msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

uint32_t get_cur_time() {
	time_t tm = time(NULL);
	
	uint32_t res = tm;
	
	return res;
}

char* timetostr(uint32_t tm) {
	time_t t = tm;
	return ctime(&t);
}

void print_range(int start, int size) {
	printf("start: %d -- size: %d\n", start, size);
}