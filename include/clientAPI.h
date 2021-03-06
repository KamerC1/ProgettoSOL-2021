#ifndef clientAPI_h
#define clientAPI_h
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../utils/util.h"
#include "../utils/conn.h"
#include "../utils/flagsAPI.h"



//in caso di errore della API, il server invia l'errno; altrimenti invia questa flag
#define API_SUCCESS 0

extern bool EN_STDOUT; //indica se stampare se abilitare -p
#define STAMPA_STDOUT(val) if(EN_STDOUT == true) {val;}

int FD_SOCK;
char SOCKNAME[MAX_BYTES_SOCKNAME];

int openConnection(const char *sockname, int msec, const struct timespec abstime);
int closeConnection(const char *sockname);
int openFile(const char* pathname, int flags);
int writeFile(const char *pathname, const char *dirname);
int readFile(const char *pathname, char **buf, size_t *size);
int readNFiles(int N, const char *dirname);
int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);
int lockFile(const char *pathname);
int unlockFile(const char *pathname);
int closeFile(const char *pathname);
int removeFile(const char *pathname);
int isPathPresent(const char *pathname);
size_t getSizeFileByte(const char *pathname);
int removeClientInfoAPI();
int copyFileToDir(const char *pathname, const char *dirname);

#endif