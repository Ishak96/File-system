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
#define DEFAULT_SIZE 100000


char *input,*input1;
int exitflag = 0;
int filepid;
int argcount = 0;
char cwd[BUFSIZE];
char* argval[ARGMAX]; // our local argc, argv

int __exit();
void __pwd(char*, int);
void __cd(char*);
void __touch(char*);
void __rm(char*);
void __mkdir(char*);
void __rmdir(char*);
void __clear();
void __ls(char*);
void __lsl(char*);
void __cp(char*, char*);
void __mv(char*, char*);
void __ln(char*, char*);
void __write(char*, char*);
void __cat(char*);
void getInput();
void screenfetch();

int main(int argc, char* argv[])
{
   if(argc!=2) {
		printf("use: %s <disk>\n",argv[0]);
		return 1;
	}

	if(initfs(argv[1],DEFAULT_SIZE) < 0) {
			fprintf(stderr,"shell : creatfile %s\n",argv[1]);
			return 1;
	}
	printf("opened emulated disk image %s\n",argv[1]);
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
        else if(strcmp(argval[0],"touch")==0)
        {
            char* filename = argval[1];
            __touch(filename);
        }
        else if(strcmp(argval[0],"rmdir")==0)
        {
            char* foldername = argval[1];
            __rmdir(foldername);
        }
		else if(strcmp(argval[0],"rm")==0)
        {
            char* filename = argval[1];
            __rm(filename);
        }
        else if(strcmp(argval[0],"clear")==0)
        {
            __clear();
        }
        else if(strcmp(argval[0],"ls")==0)
        {
            char* optional = argval[1];
            char* path_arg = NULL;
            
            if(argcount == 3){
            	if(argval[1][0] == '-'){
            		optional = argval[1];
            		path_arg = argval[2];
            	}
            	else{
            		optional = argval[2];
            		path_arg = argval[1];
            	}
            }
            else if(argcount == 2){
            	if(argval[1][0] == '-'){
            		optional = argval[1];
            		path_arg = NULL;
            	}
            	else{
            		optional = "none";
            		path_arg = argval[1];
            	}            	
            }

            if(strcmp(optional,"-l")==0)
            {
                __lsl(path_arg);
            }
            else
            {
            	__ls(path_arg);
           	}
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
        else if(strcmp(argval[0],"mv")==0)
        {
            char* file1 = argval[1];
            char* file2 = argval[2];
            if(argcount > 2 && strlen(file1) > 0 && strlen(file2) > 0)
            {
                __mv(file1,file2);
            }
            else
            {
                printf("+--- Error in mv : insufficient parameters\n");
            }
        }
        else if(strcmp(argval[0],"ln")==0)
        {
            char* file1 = argval[1];
            char* file2 = argval[2];
            if(argcount > 2 && strlen(file1) > 0 && strlen(file2) > 0)
            {
                __ln(file1,file2);
            }
            else
            {
                printf("+--- Error in ln : insufficient parameters\n");
            }
        }
        else if(strcmp(argval[0],"write")==0)
        {
            char* file1 = argval[1];
            char* file2 = argval[2];
            if(argcount > 2 && strlen(file1) > 0 && strlen(file2) > 0)
            {
                __write(file1,file2);
            }
            else
            {
                printf("+--- Error in ln : insufficient parameters\n");
            }
        }
        else if(strcmp(argval[0],"cat")==0)
        {
            char* filename = argval[1];
            __cat(filename);
        }
    }
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

/* read th e file */
void __cat(char* path)
{
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, path);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, path);
	}
	int fd = open_(tmp_cwd, 0, 0);
	if(fd < 0) {
		fprintf(stderr, "cannot write to the file\n");
		return ;
	}
	struct fs_inode ind = getInode(path);
	if(ind.hcount == 0) {
		fprintf(stderr, "cannot read inode\n");
		return;
	}
	char* data = malloc(sizeof(char) * ind.size);
	if(read_(fd, data, ind.size) < 0){
		fprintf(stderr, "cannot write to the file\n");
		return ;		
	}
	printf("%s\n", data);
	free(data);
	close_(fd);
}

/* write in a file */
void __write(char* path, char* data){
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, path);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, path);
	}
	
	int fd = open_(tmp_cwd, 0, 0);
	if(fd < 0) {
		fprintf(stderr, "cannot write to the file\n");
		return ;
	}
	if(write_(fd, data, strlen(data)) < 0){
		fprintf(stderr, "cannot write to the file\n");
		return ;		
	}
	close_(fd);
}

/* copy one file to another */
void __cp(char* file1, char* file2)
{

}

/* create a hard link */
void __ln(char* file1, char* file2)
{
	if(file1 == NULL || file2 == NULL){
		fprintf(stderr, "cannot create hard link\n");
		return ;
	}
	if(ln_(file1, file2) < 0){
		fprintf(stderr, "cannot create hard link\n");
		return ;
	}	
}

/* move one file to another */
void __mv(char* file1, char* file2)
{
	if(file1 == NULL || file2 == NULL){
		fprintf(stderr, "cannot move file\n");
		return ;
	}
	if(mv_(file1, file2) < 0){
		fprintf(stderr, "cannot move file\n");
		return ;
	}
}

/*ls -l  lists date permissions etc*/
void __lsl(char* path)
{

}

/* list cwd contents*/
void __ls(char* path)
{
	if(path == NULL){
		ls_(cwd);
	}
	else{
		char tmp_cwd[BUFSIZE] = {0};
		strcpy(tmp_cwd, cwd);
		strcat(tmp_cwd, path);
		ls_(tmp_cwd);
	}
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
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, name);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, name);
	}

	rmdir_(tmp_cwd, 1);
}

/* remove file */
void __rm(char* name){
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, name);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, name);
	}

	rm_(tmp_cwd);
}

/* Make folder */
void __mkdir(char* name)
{
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, name);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, name);
	}

	DIR_* tmp_dir = opendir_(tmp_cwd, 1, 0);
	closedir_(tmp_dir);
}

char* resolve_symlink(char* sym) {
	if(sym == NULL) {
		return NULL;
	}
	int len = strlen(sym);
	if(sym[len-1] == '/') {
		sym[len-1] = '\0';
		len--;
	}

	char* res = malloc(sizeof(char) * len);
	if(res == NULL) {
		fprintf(stderr, "malloc error at resolve_symlink\n");
	}
	int idx = 1;
	strcpy(res, "/");

	char delim[2] = "/";
	char* tok = strtok(sym, delim);
	int last_len = 0;
	while(tok != NULL) {
		if(!strcmp(".", tok)) {
			tok = strtok(NULL, delim);
			continue;
		} else if(!strcmp("..", tok)) {
			idx = (idx-last_len-1 < 1)? 1: idx-last_len-1;
		} else {
			for(int i=idx; i<idx+strlen(tok); i++) {
				res[i] = tok[i-idx];
			}
			idx += strlen(tok);
			res[idx++] = '/';
		}
		last_len = strlen(tok);
		tok = strtok(NULL, delim);
	}
	res[idx] = '\0';
	return res;
}

/*Make file*/
void __touch(char* name){
	char tmp_cwd[BUFSIZE];
	strcpy(tmp_cwd, cwd);
	
	if(tmp_cwd[strlen(tmp_cwd) - 1] == '/')
		strcat(tmp_cwd, name);
	else{
		strcat(tmp_cwd, "/");
		strcat(tmp_cwd, name);
	}

	int fd = open_(tmp_cwd, 1, 0);
	close_(fd);
}

/*change directory functionality*/
void __cd(char* path)
{
	char temp_str[BUFSIZE] = {0};
	strcat(temp_str, cwd);
	if((path == NULL) || (strlen(path) == 0)) {
		strcpy(cwd, "/");
		return;
	}

	if(path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}

	if(path[0] == '/') {
		char* temp = resolve_symlink(path);
		strcpy(cwd, temp);
		free(temp);
	} else {
		if(cwd[strlen(cwd) - 1] != '/') {
			strcat(cwd, "/");
		}
		strcat(cwd, path);
		char* temp = resolve_symlink(cwd);
		strcpy(cwd, temp);
		free(temp);
	}
	DIR_* dir = opendir_(cwd, 0, 0);
	if(dir == NULL) {
		fprintf(stderr, "directory %s does not exist\n", cwd);
		strcpy(cwd, temp_str);
		return;
	}
	closedir_(dir);
}

/*Implement basic exit*/
int __exit()
{
    exitflag = 1;
    closefs();
    return 0;
}

/* Implement pwd function in shell - 1 prints, 0 stores*/
void __pwd(char* cwdstr,int command)
{
	if(cwdstr != NULL)
    	printf("%s\n",cwdstr);
    else 
    	fprintf(stderr, "__pwd: invalid argument!\n");
}
