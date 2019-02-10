/**
 * @file manipulation_partition.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define warning(param) printf("\033[31m"); printf("%s\n", param); printf("\033[37m");
/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief create a file partition
 * @details the creation of the partition file is done according to the directory
 * @param directory the directory where the partition will be created
 */
void create_partition(char* directory) {
 int desc;
 	
 	strcat(directory, "/partition");
 	desc = open(directory, O_RDWR|O_CREAT, 0777);

 	if(desc == -1){
 		warning("open file failed");
 		exit(1);
 	}
 	write(desc, "BEGIN", 5);
 	lseek(desc, 10000000, SEEK_END);
 	write(desc, "END", 3);
 	close(desc);
}
