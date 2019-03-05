#ifndef DEVUTILS_H
#define DEVUTILS_H

#include <stdint.h>

#define FUNC_ERROR -1
#define BITS_PER_BYTE 8
#define NO_BYTES_32 4294967296

#define warning(param) printf("\033[31m"); printf("%s\n", param); printf("\033[37m");

#define SET_MINMAX(a,min,max) (((a) > min)? ((a) < max)? (a): max: min)
#define NOT_NULL(a) ((a)<=0)? 1: (a);


void die(const char* msg);
uint32_t get_cur_time();
char* timetostr(uint32_t tm);
void print_range(int start, int size);
#endif
