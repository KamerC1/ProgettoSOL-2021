#ifndef serverAPI_h
#define serverAPI_h

#include <stdbool.h>
#include "../strutture_dati/sortedList/sortedList.c"
#include "../strutture_dati/hash/icl_hash.c"
#include "../utils/flagsAPI.h"
#include "../strutture_dati/readers_writers_lock/readers_writers_lock.c"



struct file {
    char *path;
    char *fileContent;
    size_t sizeFileByte; //tiene conto anche del '\0'
    bool canWriteFile; //1: se si può fare la WriteFile(), 0 altrimenti

    //lista che memorizza l'fd dei client che hanno chiamato la open
    NodoSL *fdOpen_SLPtr;

    RwLock_t fileLock;

    pthread_mutex_t mutexFile;
};
typedef struct file File;

//mutex globale per gestire la tabella hash del file system
static pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;

//Gestione lock per la API in "lettura"
#define START_READ_LOCK                             \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    UNLOCK(&globalMutex)                            \
    rwLock_startReading(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

#define END_READ_LOCK                               \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    rwLock_endReading(&serverFile->fileLock);       \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

//Gestione lock per la API in "scrittura"
#define START_WRITE_LOCK                            \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    UNLOCK(&globalMutex)                            \
    rwLock_startWriting(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

#define END_WRITE_LOCK                              \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    rwLock_endWriting(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \


//numero di file che il server può contenere attraverso la tabella hash (dato da prendere da config.txt) [eliminare]
#define NUMFILE 10


//Funzioni che implementano le API
int writeFileServer(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
int openFileServer(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd);
int closeFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd);
int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
int readFileServer(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd);
int readNFilesServer(int N, const char *dirname, icl_hash_t *hashPtrF);
int removeFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd);

int fillStructFile(File *serverFileF);
static File *createFile(const char *pathname, int clientFd);
void stampaHash(icl_hash_t *hashPtr);
void freeFileData(void *serverFile);
int createReadNFiles(int N, icl_hash_t *hashPtrF);
int checkPathname(const char *pathname);

#endif
