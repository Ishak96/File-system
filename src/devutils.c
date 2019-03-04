#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void die(const char* msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}
