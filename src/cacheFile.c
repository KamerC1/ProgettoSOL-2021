#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

#include "../utils/util.h"
#include "../strutture_dati/queueFile/queueFile.h"
#include "../include/cacheFile.h"
#include "../include/serverAPI.h"
#include "../strutture_dati/hash/icl_hash.h"

//ritorna NULL in caso di fallimento
ServerStorage *createStorage(size_t maxStorageBytesF, size_t maxStorageFilesF, short fileReplacementAlgF)
{
    CSN(maxStorageBytesF <= 0 || maxStorageFilesF <= 0, "createStorage: argomenti sbagliati", EINVAL)
    CSN(fileReplacementAlgF != 0, "createStorage: fileReplacementAlgF != 0", EINVAL)



    ServerStorage *storage;
    RETURN_NULL_SYSCALL(storage, malloc(sizeof(ServerStorage)), "createStorage: serverFile = malloc(sizeof(File))")

    storage->maxStorageBytes = maxStorageBytesF;
    storage->maxStorageFiles = maxStorageFilesF;
    storage->currentStorageBytes = 0;
    storage->currentStorageFiles = 0;

    storage->fileReplacementAlg = fileReplacementAlgF;
    storage->FIFOtestaPtr = NULL;
    storage->FIFOcodaPtr = NULL;

    SYSCALL_NOTZERO(pthread_mutex_init(&(storage->globalMutex), NULL), "createStorage: pthread_mutex_init")

    SYSCALL_NOTZERO(pthread_mutex_init(&(storage->mutexRemoveFile), NULL), "createStorage: pthread_mutex_init")
    SYSCALL_NOTZERO(pthread_cond_init(&(storage->condRemoveFile), NULL), "rwLock_init: pthread_cond_init")
    storage->isRemovingFile = false;
    storage->isHandlingAPI = false;

    storage->fileSystem = icl_hash_create(maxStorageFilesF, NULL, NULL);
    CSAN(storage->fileSystem == NULL, "createStorage: icl_hash_create", EPERM, free(storage))

    storage->logFile = NULL;


    storage->maxByteStored = 0;
    storage->maxFileStored = 0;
    storage->numVictims = 0;

    return storage;
}

//aggiorana la dimensione dello storage, sia lo spazio che i numero dei file, quando viene aggiunto un nuovo file.
// Se necessario, elimina i file già presenti
//se fallisce, termina perché il file system rimarrebbe in uno stato incosistente
void addFile2Storage(File *serverFile, ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF)
{
    NULL_SYSCALL(serverFile, "updateStorage: serverFile == NULL")
    NULL_SYSCALL(storage, "updateStorage: storage == NULL")
    assert(serverFile->sizeFileByte <= storage->maxStorageBytes); //Questa funzione non deve essere chiamata se non la condizione nell'assert

    if(storage->currentStorageFiles >= storage->maxStorageFiles)
    {
        puts("RIMPIAZZZOOOOOOOOOOOOOOOO FILES"); //[ELIMINARE]
        FIFO_ReplacementAlg(storage, testaPtrF, codaPtrF, serverFile->path);
    }

    //Aggiornamento storage
    storage->currentStorageFiles++;
    storage->maxFileStored++;

    //aggiornamento lista dei "file creati (politica FIFO)"
    pushStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), serverFile->path);
}

//aggiorna la dimensione dello storage quando si scrive su un file - modifica solo lo spazio.
// Se necessario, elimina i file già presenti
//se fallisce, termina perché il file system rimarrebbe in uno stato incosistente
void addBytes2Storage(File *serverFile, ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF)
{
    NULL_SYSCALL(serverFile, "addBytes2Storage: serverFile == NULL")
    NULL_SYSCALL(storage, "addBytes2Storage: storage == NULL")
    assert(serverFile->sizeFileByte <= storage->maxStorageBytes); //Questa funzione non deve essere chiamata se non la condizione nell'assert

    size_t possibleCurrentBytes = storage->currentStorageBytes + serverFile->sizeFileByte;
    while(possibleCurrentBytes > storage->maxStorageBytes)
    {
        puts("RIMPIAZZOOOOOOOOOOOOOOOOOOo BYTES"); //[ELIMINARE]

//        printf("\n\n\n\n\n\n\nSERVERFILE PRIMA: %s\n\n\n\n\n\n\n", serverFile->path);


        FIFO_ReplacementAlg(storage, testaPtrF, codaPtrF, serverFile->path);
        possibleCurrentBytes = storage->currentStorageBytes + serverFile->sizeFileByte;
    }

    //Aggiornamento storage
    storage->currentStorageBytes += serverFile->sizeFileByte;
    storage->maxByteStored += serverFile->sizeFileByte;
}

//Elimina un file dallo storage e ne restituisce la dimensione (la politica di rimpiazzamento è FIFO)
//pathnameCaller è il pathname del file che ha causato la rimozione
void FIFO_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, char *pathnameCaller)
{
    assert(storage != NULL);
    assert(storage->FIFOtestaPtr != NULL);

    char *pathnameFile2Remove = storage->FIFOtestaPtr->stringa;
    //Il primo elemento della FIFO protrebbe essere il file chiamante: in questo caso, prendiamo il secondo file nella coda
    if(strcmp(pathnameFile2Remove, pathnameCaller) == 0)
    {
        assert(swapFirstWithSecond(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr)) == 0);
    }
    pathnameFile2Remove = popString(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr));


    //se fallisce, termino perché il file system rimarrebbe in uno stato incosistente
    File *serverFile = icl_hash_find(storage->fileSystem, (void *) pathnameFile2Remove);
    NULL_SYSCALL(serverFile, "FIFO_ReplacementAlg: icl_hash_find")


    size_t temp_sizeFileByte = serverFile->sizeFileByte;

    assert(serverFile->path != NULL);
    SYSCALL(icl_hash_delete(storage->fileSystem, (void *) pathnameFile2Remove, free, NULL), "FIFO_ReplacementAlg: icl_hash_delete")

    pushFile(testaPtrF, codaPtrF, serverFile);
    assert(serverFile != NULL);


    free(pathnameFile2Remove);

    storage->currentStorageFiles--;
    storage->currentStorageBytes -= temp_sizeFileByte;
    storage->numVictims++;
}

int copyFile2Dir(NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, const char *dirname)
{
    //==Calcola il path dove lavora il processo==
    //calcolo la lunghezza del path [eliminare]
    char *processPath = getcwd(NULL, PATH_MAX);
    CS(processPath == NULL, "copyFile2Dir: getcwd", errno)
    int pathLength = strlen(processPath); //il +1 è già compreso in MAX_PATH_LENGTH
    REALLOC(processPath, pathLength + 1) //getcwd alloca "MAX_PATH_LENGTH" bytes

    //cambio path [eliminare]
    DIR *directory = opendir(dirname);
    CSA(directory == NULL, "directory = opendir(dirname): ", errno, free(processPath))
    //la mutua  esclusione è già stata acquisita in writeFileServer o appendToFileServer
    CSA(chdir(dirname) == -1, "copyFile2Dir: chdir(dirname)", errno, free(processPath); CLOSEDIR(directory))

    File *serverFile = popFile(testaPtrF, codaPtrF);

    while(serverFile != NULL)
    {
        //se falisce, si scrive copia il prossimo file
        wrtieFile(serverFile);

        freeFileData(serverFile);
        serverFile = popFile(testaPtrF, codaPtrF);
    }

    CSA(chdir(processPath) == -1, "readNFiles: chdir(processPath)", errno, free(processPath); CLOSEDIR(directory))

    free(processPath);
    CLOSEDIR(directory)

    return 0;
}

//Crea un file vi copia il contenuto di serverFile - funzione di supporto per copyFile2Dir
int wrtieFile(File *serverFile)
{
    assert(serverFile != NULL);

    //==Creazione file e scrittura su di esso==
    //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
    FILE *diskFile = fopen(basename(serverFile->path), "w");

    CS(diskFile == NULL, "copyFile2Dir: fopen()", errno)
    if (serverFile->fileContent != NULL && fputs(serverFile->fileContent, diskFile) == EOF)
    {
        PRINT("copyFile2Dir: fputs");
        SYSCALL_NOTZERO(fclose(diskFile), "copyFile2Dir: fclose() - termino processo")
    }
    SYSCALL_NOTZERO(fclose(diskFile), "createReadNFiles: fclose() - termino processo")

    return 0;
}

void stampaStorage(ServerStorage *storage)
{
    assert(storage != NULL);
    LOCK(&(storage->globalMutex))

    printf("maxStorageBytes: %ld\n", storage->maxStorageBytes);
    printf("currentStorageBytes: %ld\n", storage->currentStorageBytes);
    printf("maxStorageFiles: %ld\n", storage->maxStorageFiles);
    printf("currentStorageFiles: %ld\n", storage->currentStorageFiles);
    printf("maxFileStored: %ld\n", storage->maxFileStored);
    printf("maxByteStored: %ld\n", storage->maxByteStored);
    printf("numVictims: %ld\n", storage->numVictims);


    stampaQueueStringa(storage->FIFOtestaPtr);

    UNLOCK(&(storage->globalMutex))
}
