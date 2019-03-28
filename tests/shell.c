/**
 * @file shell.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main dirent functions
 */
#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <fcntl.h>

#include <fs.h>
#include <ui.h>
#include <disk.h>
#include <io.h>
#include <devutils.h>
#include <dirent.h>

#define DEF "\x1B[0m"
#define ARGMAX 10
#define BUFSIZE 1000


char *input,*input1;
int exitflag = 0;
int filepid;
int argcount = 0;
char cwd[BUFSIZE];
char* argval[ARGMAX]; // our local argc, argv

int __exit();
void __pwd(char*, int);
void __cd(char*);
void __mkdir(char*);
void __rmdir(char*);
void __clear();
void nameFile(struct dirent*, char*);
void __ls();
void __lsl();
void __cp(char*, char*);
void getInput();
void screenfetch();

int main(int argc, char* argv[])
{
   /*if(argc!=3) {
		printf("use: %s <disk> <nblocks>\n",argv[0]);
		return 1;
	}

	if(initfs(argv[1],atoi(argv[2])) < 0) {
		fprintf(stderr,"shell : creatfile %s\n",argv[1]);
		return 1;
	}
	printf("opened emulated disk image %s\n",argv[1]);*/
	strcpy(cwd, "/");

    while(exitflag==0)
    {
        printf("%s%s ~> ",DEF,cwd);
        getInput();

        if(strcmp(argval[0],"exit")==0 || strcmp(argval[0],"z")==0)
        {
            __exit();
        }
        else if(strcmp(argval[0],"pwd")==0)
        {
            __pwd(cwd,1);
        }
        else if(strcmp(argval[0],"cd")==0)
        {
            char* path = argval[1];
            __cd(path);
        }
        else if(strcmp(argval[0],"mkdir")==0)
        {
            char* foldername = argval[1];
            __mkdir(foldername);
        }
        else if(strcmp(argval[0],"rmdir")==0)
        {
            char* foldername = argval[1];
            __rmdir(foldername);
        }
        else if(strcmp(argval[0],"clear")==0)
        {
            __clear();
        }
        else if(strcmp(argval[0],"ls")==0)
        {
            char* optional = argval[1];
            if(strcmp(optional,"-l")==0)
            {
                __lsl();
            }
            else __ls();
        }
        else if(strcmp(argval[0],"cp")==0)
        {
            char* file1 = argval[1];
            char* file2 = argval[2];
            if(argcount > 2 && strlen(file1) > 0 && strlen(file2) > 0)
            {
                __cp(file1,file2);
            }
            else
            {
                printf("+--- Error in cp : insufficient parameters\n");
            }
        }
    }
   // closedir_(rootdir);
}

/*get input containing spaces and tabs and store it in argval*/
void getInput()
{
    fflush(stdout); // clear all previous buffers if any
    input = NULL;
    size_t buf = 0;
    getline(&input,&buf,stdin);
    // Copy into another string if we need to run special executables
    input1 = (char *)malloc(strlen(input) * sizeof(char));
    strncpy(input1,input,strlen(input));
    argcount = 0;
    while((argval[argcount] = strsep(&input, " \t\n")) != NULL && argcount < ARGMAX-1)
    {
        // do not consider "" as a parameter
        if(sizeof(argval[argcount])==0)
        {
            free(argval[argcount]);
        }
        else argcount++;
    }
    free(input);
}

/* copy one file to another */
void __cp(char* file1, char* file2)
{

}

/* Just a fancy name printing function*/
void nameFile(struct dirent* name,char* followup)
{

}

/*ls -l  lists date permissions etc*/
void __lsl()
{

}

/* list cwd contents*/
void __ls()
{

}

/* clear the screen*/
void __clear()
{
    const char* blank = "\e[1;1H\e[2J";
    write(STDOUT_FILENO,blank,12);
}

/* remove folder */
void __rmdir(char* name)
{

}

/* Make folder */
void __mkdir(char* name)
{

}

/*change directory functionality*/
void __cd(char* path)
{
	strcat(cwd, path);
}

/*Implement basic exit*/
int __exit()
{
    exitflag = 1;
    return 0; // return 0 to parent process in run.c
}

/* Implement pwd function in shell - 1 prints, 0 stores*/
void __pwd(char* cwdstr,int command)
{
	if(cwdstr != NULL)
    	printf("%s\n",cwdstr);
    else 
    	fprintf(stderr, "__pwd: invalid argument!\n");
}