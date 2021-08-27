#ifndef serverAPI_h
#define serverAPI_h

#include <stdbool.h>
#include "../strutture_dati/sortedList/sortedList.h"
#include "../strutture_dati/queueInt/queue.h"
#include "../strutture_dati/queueChar/queueChar.h"
#include "../strutture_dati/hash/icl_hash.h"
#include "../utils/flagsAPI.h"
#include "../strutture_dati/readers_writers_lock/readers_writers_lock.h"




struct file {
    char *path;
    char *fileContent;
    size_t sizeFileByte; //tiene conto anche del '\0'
    bool canWriteFile; //1: se si può fare la WriteFile(), 0 altrimenti

    NodoSL *fdOpen_SLPtr; //lista che memorizza l'fd dei client che hanno chiamato la open

    int lockFd; //fd di chi ha la lock (-1 se nessuno ha la lock)
    NodoQi *fdLock_TestaPtr; //fd in attesa di acquisire la lock (è una coda)
    NodoQi *fdLock_CodaPtr; //fd in attesa di acquisire la lock (è una coda)

    RwLock_t fileLock; //struttura dati per gestire la concorrenza

    time_t LRU_time; //tempo per implementare l'algoritmo di sostituizione: LRU
    size_t accessFile_count; //conta il numero di accessi al file (serve per LFU e MFU)
};
typedef struct file File;

struct serverStorage
{
    size_t maxStorageBytes; //massimo numero di bytes usabili nel server
    size_t maxStorageFiles; //massimo numero di files che server può contenere
    size_t currentStorageBytes; //numero di bytes attualmente usati nel server
    size_t currentStorageFiles; //numero di files attualmente memorizzati nel server

    short fileReplacementAlg; //0->FIFO, 1->LRU, 2->LFU
    //Struttura dati per gestire la politica di rimozione di tipo FIFO (contiene i file attualmente aperti)
    NodoQi_string *FIFOtestaPtr;
    NodoQi_string *FIFOcodaPtr;

    pthread_mutex_t globalMutex;

    bool isRemovingFile; //è true se si sta attualemente elimindo il file
    bool isHandlingAPI; //è true se si sta gestendo una funzione della API
    pthread_cond_t condRemoveFile; //Da usare quando per la removeFile

    icl_hash_t *fileSystem; //tabella hash che memorizza l'insieme dei file

    FILE *logFile; //file di log dove scrivere le operazioni dello storage


    size_t maxFileStored; //massimo numero di file memorizzati
    size_t maxByteStored; //massimo numero di byte memorizzati
    size_t numVictims;  //numero di vittime dall'algoritmo di rimpiazzamento
};
typedef struct serverStorage ServerStorage;

//mutex globale per gestire la tabella hash del file system

//Ritorna errore se chi chiama la funzione dell'API NON possiede la lock (ins rappresenta un insieme di istruzioni)
#define IS_FILE_LOCKED(ins)                                         \
if(serverFile->lockFd != -1 && serverFile->lockFd != clientFd)      \
{                                                                   \
    errno = EACCES;                                                 \
    PRINT("File lockato");                                          \
    ins;                                                            \
    return -1;                                                      \
}                                                                   \


//Gestione lock per la API in "lettura"
#define START_READ_LOCK                             \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    UNLOCK(&(storage->globalMutex))                 \
    rwLock_startReading(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

#define END_READ_LOCK                               \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    rwLock_endReading(&serverFile->fileLock);       \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

//Gestione lock per la API in "scrittura"
#define START_WRITE_LOCK                            \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    UNLOCK(&(storage->globalMutex))                 \
    rwLock_startWriting(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

#define END_WRITE_LOCK                              \
    LOCK(&(serverFile->fileLock.mutexFile))         \
    rwLock_endWriting(&serverFile->fileLock);     \
    UNLOCK(&(serverFile->fileLock.mutexFile))       \

#define WAKES_UP_REMOVE                             \
            storage->isHandlingAPI = false;         \
            SIGNAL(&(storage->condRemoveFile))      \

#define WAKES_UP_API                             \
    storage->isRemovingFile = false;            \
    SIGNAL(&(storage->condRemoveFile))          \

//numero di file che il server può contenere attraverso la tabella hash (dato da prendere da config.txt) [eliminare]
#define NUMFILE 10


//Funzioni che implementano le API
long writeFileServer(const char *pathname, const char *dirname, ServerStorage *storage, int clientFd);
int openFileServer(const char *pathname, int flags, ServerStorage *storage, int clientFd);
int closeFileServer(const char *pathname, ServerStorage *storage, int clientFd);
int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, ServerStorage *storage, int clientFd);
long readFileServer(const char *pathname, char **buf, size_t *size, ServerStorage *storage, int clientFd);
long copyFileToDirServer(const char *pathname, const char *dirname, ServerStorage *storage, int clientFd);
int readNFilesServer(int N, const char *dirname, ServerStorage *storage, int clientFd, unsigned long long int *bytesLetti);
int removeFileServer(const char *pathname, ServerStorage *storage, int clientFd);
int lockFileServer(const char *pathname, ServerStorage *storage, int clientFd);
int unlockFileServer(const char *pathname, ServerStorage *storage, int clientFd);
size_t getSizeFileByteServer(const char pathname[], ServerStorage *storage, int clientFd);

//static int fillStructFile(File *serverFileF, ServerStorage *storage, const char *dirname);
//static File *createFile(const char *pathname, int clientFd);
void stampaHash(ServerStorage *storage);
void freeFileData(void *serverFile);
//static int copyFileToDirHandler(File *serverFile, const char dirname[]);
//int createReadNFiles(int N, ServerStorage *storage, int clientFd, unsigned long long int *bytesLetti);
//static int creatFileAndCopy(File *serverFile);
int gestioneApi_removeClientInfoAPI(int fdAcceptF);
int removeClientInfo(ServerStorage *storage, int clientFd);
void freeStorage(ServerStorage *storage);
int isPathPresentServer(const char pathname[], ServerStorage *storage, int clientFd);


#endif