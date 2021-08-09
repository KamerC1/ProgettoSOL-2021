#ifndef clientAPI_h
#define clientAPI_h
#include <stdlib.h>
#include <string.h>
#include "../utils/util.h"
#include "../utils/conn.h"
#include "../utils/utilsPathname.c"
#include "../utils/flagsAPI.h"


//in caso di errore della API, il server invia l'errno; altrimenti invia questa flag
#define API_SUCCESS 0

int FD_SOCK;
char SOCKNAME[MAX_BYTES_SOCKNAME];

int openConnection(const char *sockname, int msec, const struct timespec abstime);
int closeConnection(const char *sockname);
int openFile(const char* pathname, int flags);
int writeFile(const char *pathname, const char *dirname);
int readFile(const char *pathname, char **buf, size_t *size);
int readNFiles(int N, const char *dirname);
int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);
int closeFile(const char *pathname);
int removeFile(const char *pathname);

#endif