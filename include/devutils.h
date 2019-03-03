#ifndef DEVUTILS_H
#define DEVUTILS_H

#define warning(param) printf("\033[31m"); printf("%s\n", param); printf("\033[37m");

void die(const char* msg);

#endif
