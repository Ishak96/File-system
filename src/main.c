/**
 * @file main.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <meminit.h>

/**
* @author ABDELMOUMENE Djahid 
* @author AYAD Ishak
* @brief misuse of agrs
* @param prog name of the programme 
*/
static void	usage(char* prog){
  fprintf(stderr, "usage: %s [-p arg]\n", prog);
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

 	if(argc >= 2)
	 	while((opt = getopt(argc, argv, "p:")) != -1){
	 		switch(opt){
	 			case 'p':
	 				creatfile(optarg);
	 				break;
	 			default:
	 				usage(argv[0]);
	 				break;
	 		}
	 	}
	else{
		getcwd(cwd, sizeof(cwd));
		creatfile(cwd);
	}
	return 0;
}
