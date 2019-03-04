#ifndef DEVUTILS_H
#define DEVUTILS_H

#define warning(param) printf("\033[31m"); printf("%s\n", param); printf("\033[37m");
#define FUNC_ERROR -1
void die(const char* msg);

#endif
