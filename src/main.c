/**
 * @file main.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fs.h>


/**
* @author ABDELMOUMENE Djahid 
* @author AYAD Ishak
* @brief misuse of agrs
* @param prog name of the programme 
*/
static void	usage(char* prog){
  fprintf(stderr, "usage: %s [-s arg] [-p arg]\n", prog);
  exit(1);
}
/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main function
 */
int main(int argc, char** argv) {
 int opt;
 char cwd[4012];
 size_t size = 4000;

 	if(argc >= 2){
	 	while((opt = getopt(argc, argv, "s:p:")) != -1){
	 		switch(opt){
	 			case 's':
	 				size = atoi(optarg);
	 				break;
	 			case 'p':
	 				creatfile(optarg, size);
	 				break;
	 			default:
	 				usage(argv[0]);
	 				break;
	 		}
	 	}
	}
	else{
		getcwd(cwd, sizeof(cwd));
		strcat(cwd, "/partition");

		creatfile(cwd, size);
	}
	return 0;
}
