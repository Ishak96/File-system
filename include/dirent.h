/**
 * @file dirent.h
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief dirent.h - format of directory entries
 * @details main filesystem structs and prototypes
 */
#ifndef DIRENT_H
#define DIRENT_H

#include <wchar.h>

struct _WDIR {
    struct _wdirent ent;                        /* Current directory entry */
    int cached;                                 /* True if data is valid */
    wchar_t *patt;                              /* Initial directory name */
};
typedef struct _WDIR _WDIR;

struct dirent {
    long d_ino;                                 /* Always zero */
    int d_type;                                 /* File type */
    char d_name[PATH_MAX];                      /* File name */
};
typedef struct dirent dirent;

struct DIR {
    struct dirent ent;
    struct _WDIR *wdirp;
};
typedef struct DIR DIR;

DIR *opendir (const char *dirname);
struct dirent *readdir (DIR *dirp);
int closedir (DIR *dirp);
void rewinddir (DIR* dirp);

#endif