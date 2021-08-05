#ifndef serverAPI_h
#define serverAPI_h

#include <stdbool.h>
#include "../sortedList/sortedList.c"
#include "../hash/icl_hash.c"

#endif

struct file {
    char *path;
    char *fileContent;
    size_t sizeFileByte; //tiene conto anche del '\0'
    bool canWriteFile; //1: se si può fare la WriteFile(), 0 altrimenti

    //lista che memorizza l'fd dei client che hanno chiamato la open
    Nodo *fdOpen_SLPtr;

};
typedef struct file File;

int writeFile(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
int openFile(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd);
int closeFile(const char *pathname, icl_hash_t *hashPtrF, int clientFd);
int appendToFile(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
int readFile(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd);
int readNFiles(int N, const char *dirname, icl_hash_t *hashPtrF);