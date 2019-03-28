/**
 * @file shell.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main dirent functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <io.h>
#include <fs.h>
#include <devutils.h>
#include <disk.h>
#include <dirent.h>
#include <ui.h>

#define DEF "\x1B[0m"
#define ARGMAX 100
#define BUFSIZE 1000
#define DEFAULT_SIZE 100000


char *input,*input1;
int exitflag = 0;
int filepid;
int argcount = 0;
char cwd[BUFSIZE];
char** argval; // our local argc, argv

int __exit();
char* getPath(char* cur, char* path) ;
void __pwd(char*);
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
   int format = 0;
   argval =( char ** ) malloc ( ARGMAX * sizeof ( char *) ) ;
   if(argc < 2) {
		printf("use: %s [-f --format] <disk>\n",argv[0]);
		return 1;
	}
	for(int opt=1; opt<argc; opt++) {
		if(!strcmp("-f", argv[opt]) || !strcmp("--format", argv[opt])) {
			format = 1;
		}
	}
	if(initfs(argv[1], DEFAULT_SIZE, format) < 0) {
			fprintf(stderr,"shell : creatfile %s\n",argv[1]);
			return 1;
	}
	printf("opened emulated disk image \"%s\"\n",argv[1]);
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
            __pwd(cwd);
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
        else if(strlen(argval[0]))
        {
			fprintf(stderr, "Invalid command \"%s\"\n", argval[0]);
		}
    }
}


/**
* @breif get input 
* @details get input containing spaces and tabs and store it in argval
*/
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
/**
* @breif resolve symlink
* @details resolve symlink for .. and . case
* @param sym 	the symlink
* @return string without .. and .
*/
char* resolve_symlink(char* sym) {
	if(sym == NULL) {
		return NULL;
	}
	int len = strlen(sym);
	if(sym[len-1] == '/') {
		sym[len-1] = '\0';
		len--;
	}

	char* res = malloc(sizeof(char) * (len+5));
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

char* getPath(char* cur, char* path) {
	char *res = malloc(sizeof(char*) * (strlen(cur) + strlen(path)));
	strcpy(res, cur);
	if((path == NULL) || (strlen(path) == 0)) {
		strcpy(res, "/");
		return NULL;
	}

	if(path[strlen(path) - 1] == '/') {
		path[strlen(path) - 1] = '\0';
	}

	if(path[0] == '/') {
		char* temp = resolve_symlink(path);
		strcpy(res, temp);
		free(temp);
	} else {
		if(res[strlen(res) - 1] != '/') {
			strcat(cur, "/");
		}
		strcat(res, path);
		char* temp = resolve_symlink(res);
		strcpy(res, temp);
		free(temp);
	}
	return res;
}
/**
* @brief read file
* @param path 	the file path to read 
*/
void __cat(char* path)
{
	char tmp_cwd[BUFSIZE];
	char* new_path = getPath(cwd, path);
	if(new_path == NULL) {
		fprintf(stderr, "cannot resolve path\n");
		return;
	}
	strcpy(tmp_cwd, new_path);
	free(new_path);

	int fd = open_(tmp_cwd, 0, 0);
	if(fd < 0) {
		fprintf(stderr, "cannot write to the file\n");
		return ;
	}
	struct fs_inode ind = getInode(tmp_cwd);
	if(ind.hcount == 0) {
		fprintf(stderr, "cannot read inode\n");
		return;
	}
	
	char* data = malloc(sizeof(char) * (ind.size+1));
	if(read_(fd, data, ind.size+1) < 0){
		fprintf(stderr, "cannot write to the file\n");
		return ;		
	}
	printf("%s\n", data);
	free(data);
	close_(fd);
}

/**
* @breif write in a file 
* @param path 	the file path to write in
* @param data	string of characters to write
*/
void __write(char* path, char* data) {
	char tmp_cwd[BUFSIZE];
	char* new_path = getPath(cwd, path);
	if(new_path == NULL) {
		fprintf(stderr, "cannot resolve path\n");
		return;
	}
	strcpy(tmp_cwd, new_path);
	free(new_path);
	
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

/**
* @breif copy one file to another 
* @param file1	name of the first file
* @param file2 name of the second file
*/
void __cp(char* file1, char* file2)
{
	if(file1 == NULL || file2 == NULL){
		fprintf(stderr, "cannot copy file\n");
		return ;
	}
	if(cp_(file1, file2) < 0){
		fprintf(stderr, "cannot move file\n");
		return ;
	}
}

/** 
* @breif create a hard link 
* @param file1	name of the first file
* @param file2 name of the second file
*/
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

/**
* @breif move one file to another 
* @param file1	name of the first file
* @param file2 name of the second file
*/
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

/**
* @breif ls -l  lists date permissions etc
* @param path 	path of the folder
*/
void __lsl(char* path)
{
	char tmp_cwd[BUFSIZE] = {0};
	if(path == NULL){
		strcpy(tmp_cwd, cwd);
	}
	else{
		strcpy(tmp_cwd, cwd);
		strcat(tmp_cwd, path);
	}

	lsl_(tmp_cwd);
}

/**
* @breif list cwd contents
* @param path 	path of the folder
*/
void __ls(char* path)
{
	char tmp_cwd[BUFSIZE] = {0};
	if(path == NULL){
		strcpy(tmp_cwd, cwd);
	}
	else{
		strcpy(tmp_cwd, cwd);
		strcat(tmp_cwd, path);
	}

	ls_(tmp_cwd);
}

/** 
* @breif clear the screen
*/
void __clear()
{
    const char* blank = "\e[1;1H\e[2J";
    write(STDOUT_FILENO,blank,12);
}

/**
* @breif remove folder 
* @param name	dire name
*/
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

/**
* @breif remove file 
* @param name	file name
*/
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

/** 
* @breif Make folder 
* @param name	dir name
*/
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


/**
* @breif Make file
* @param name	file name
*/
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
	if(fd < 0) {
		fprintf(stderr, "could not create file %s\n", name);
		return;
	}
	close_(fd);
}

/**
* @breif change directory functionality
* @param path 	new path
*/
void __cd(char* path)
{
	if(path == NULL || strlen(path) == 0){
		strcpy(cwd, "/");
	}
	else{
		char temp_str[BUFSIZE] = {0};
		strcat(temp_str, cwd);

		char* new_path = getPath(cwd, path);
		if(new_path == NULL) {
			fprintf(stderr, "cannot resolve path\n");
			return;
		}
		strcpy(cwd, new_path);
		free(new_path);
		DIR_* dir = opendir_(cwd, 0, 0);
		if(dir == NULL) {
			fprintf(stderr, "directory %s does not exist\n", cwd);
			strcpy(cwd, temp_str);
			return;
		}
		closedir_(dir);
	}
}

/**
* @breif Implement basic exit
* @return 0
*/
int __exit()
{
    exitflag = 1;
    free(argval);
    closefs();
    return 0;
}

/**
* @breif Implement pwd function in shell - 1 prints, 0 stores
* @param cwdstr 	Curret Working Directory
*/
void __pwd(char* cwdstr)
{
	if(cwdstr != NULL)
    	printf("%s\n",cwdstr);
    else 
    	fprintf(stderr, "__pwd: invalid argument!\n");
}
