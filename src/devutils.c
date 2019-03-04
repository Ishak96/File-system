#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
/**
 * @brief utility function to terminate program
 */
void die(const char* msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}
