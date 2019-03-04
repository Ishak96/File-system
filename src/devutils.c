#include <string.h>
#include <stdlib.h>
/**
 * @brief utility function to terminate program
 */
void die(const char* msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}
