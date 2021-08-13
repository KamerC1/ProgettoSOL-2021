#include <stdio.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <assert.h>

#include "../utils/util.h"
#include "../utils/utilsPathname.c"
#include "../include/serverAPI.h"
#include "../strutture_dati/sortedList/sortedList.h"
#include "../strutture_dati/queueInt/queue.h"
#include "../strutture_dati/queueChar/queueChar.h"
#include "../strutture_dati/queueFile/queueFile.h"
#include "../include/cacheFile.h"
#include "../strutture_dati/readers_writers_lock/readers_writers_lock.h"
#include "../strutture_dati/hash/icl_hash.h"
#include "../include/log.h"

static void setLockFile(File *serverFile, int clientFd);
static void setUnlockFile(File *serverFile);
void removeLock(File *serverFile, int clientFd);

//int main()
//{
//
//}

//da finire [controllare]
int openFileServer(const char *pathname, int flags, ServerStorage *storage, int clientFd) {
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s [flags: %d]\n", "openFile", flags))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    CSA(storage == NULL, "openFile: storage == NULL", EINVAL, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);

    NodoQi_file *testaFilePtrF = NULL;
    NodoQi_file *codaFilePtrF = NULL;
    switch (flags) {
        case O_OPEN:
            CSA(serverFile == NULL, "O_OPEN: File non presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK

            //clientFd ha già aperto la lista
            CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_OPEN: file già aperto dal client", EPERM, ESOP("OpenFile", 0); END_WRITE_LOCK)

            insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
            serverFile->canWriteFile = 1;

            END_WRITE_LOCK

            ESOP("OpenFile", 1)
            break;

        case O_CREATE:
            CSA(serverFile != NULL, "O_CREATE: File già presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE: impossibile creare la entry nella hash", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK

            addFile2Storage(serverFile, storage, &testaFilePtrF, &codaFilePtrF);

            //Scrittura sul file di log
            if(testaFilePtrF != NULL)
            {
                fprintf(storage->logFile, "File espulsi: \n");
                writePathToFile(testaFilePtrF, storage->logFile);
                freeQueueFile(&testaFilePtrF, &codaFilePtrF);
            }


            NULL_SYSCALL(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile), "O_CREATE: impossibile inserire File nella hash")
            serverFile->canWriteFile = 1;

            ESOP("OpenFile", 1)
            END_WRITE_LOCK
            break;

        case O_LOCK:
            CSA(serverFile == NULL, "O_LOCK: File non presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK

            CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_LOCK: file già aperto dal client", EPERM, END_WRITE_LOCK)
            insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
            setLockFile(serverFile, clientFd);
            serverFile->canWriteFile = 1;

            ESOP("OpenFile", 1)
            END_WRITE_LOCK
            break;

        case (O_CREATE + O_LOCK):
            CSA(serverFile != NULL, "O_CREATE + O_LOCK: File già presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE + O_LOCK: impossibile creare la entry nella hash", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK

            addFile2Storage(serverFile, storage, &testaFilePtrF, &codaFilePtrF);

            //Scrittura sul file di log
            if(testaFilePtrF != NULL)
            {
                fprintf(storage->logFile, "File espulsi: \n");
                writePathToFile(testaFilePtrF, storage->logFile);
                freeQueueFile(&testaFilePtrF, &codaFilePtrF);
            }

            NULL_SYSCALL(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile),
                "O_CREATE + O_LOCK: impossibile inserire File nella hash")

            serverFile->lockFd = clientFd;
            serverFile->canWriteFile = 1;

            ESOP("OpenFile", 1)
            END_WRITE_LOCK
            break;

        default:
            //la flag non corrisponde
            PRINT("flag sbagliata")

            ESOP("OpenFile", 0)
            errno = EINVAL;
            UNLOCK(&(storage->globalMutex))
            return -1;
            break;
    }


    return 0;
}

//Funzione di supporto a openFIle: alloca e inizializzata una struttura File
static File *createFile(const char *pathname, int clientFd) {
    CSN(checkPathname(pathname), "createFile: pathname sbaglito", EINVAL)
    CSN(clientFd < 0, "clientFd < 0", EINVAL)


    File *serverFile;
    RETURN_NULL_SYSCALL(serverFile, malloc(sizeof(File)), "createFile: serverFile = malloc(sizeof(File))")

    //--Inizializzo mutex--:
    rwLock_init(&(serverFile->fileLock));

    //--inserimento pathname--
    int strlenPath = strlen(pathname);
    RETURN_NULL_SYSCALL(serverFile->path, malloc(sizeof(char) * strlenPath + 1),
                        "createFile: malloc(sizeof(char) * strlenPath + 1)")
    strncpy(serverFile->path, pathname, strlenPath + 1);

    serverFile->fileContent = NULL;
    serverFile->sizeFileByte = 0;
    serverFile->canWriteFile = 0; //viene impostato a 1 alla fine della openFile

    //--inserimento fdOpen--
    serverFile->fdOpen_SLPtr = NULL;
    insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);

    //Inizializzo fdLock
    serverFile->lockFd = -1;
    serverFile->fdLock_TestaPtr = NULL;
    serverFile->fdLock_CodaPtr = NULL;



    return serverFile;
}

int readFileServer(const char *pathname, char **buf, size_t *size, ServerStorage *storage, int clientFd) {
    //in caso di errore, buf = NULL, size = 0
    *size = 0;
    *buf = NULL;

    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname), "readFileServer: pathname sbaglito", EINVAL, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))
    DIE(fprintf(storage->logFile, "Operazione: %s\n", "writeFile"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    CSA(storage == NULL, "readFileServer: storage == NULL", EINVAL, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CSA(serverFile == NULL, "il file non è presente nella hash table", EPERM, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))

    START_READ_LOCK

    IS_FILE_LOCKED(END_READ_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "il file non è stato aperto da clientFd", EPERM, ESOP("readFile", 0); END_READ_LOCK)


    *size = serverFile->sizeFileByte;
    RETURN_NULL_SYSCALL(*buf, malloc(*size), "readFileServer: *buf = malloc(*size)")
    memset(*buf, '\0', *size);

    strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte);

    //readFile terminata con successo ==> non posso più fare la writeFile
    serverFile->canWriteFile = 0;

    DIE(fprintf(storage->logFile, "Dim buffer inviato: %ld - Buffer: %s\n", *size, *buf))
    ESOP("readFile", 1)

    END_READ_LOCK

    return 0;
}

int readNFilesServer(int N, const char *dirname, ServerStorage *storage, int clientFd)
{
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(dirname == NULL, "readNFiles: dirname == NULL", EINVAL, ESOP("readNFiles", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s [N: %d]\n", "readNFiles", N))
    DIE(fprintf(storage->logFile, "dirname: %s\n", dirname))

    CSA(storage == NULL, "readNFileServer: storage == NULL", EINVAL, ESOP("readNFiles", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    //imposta il numero di file da leggere
    if (N <= 0 || hashPtrF->nentries < N)
        N = hashPtrF->nentries;

    //==Calcola il path dove lavora il processo==
    //calcolo la lunghezza del path [eliminare]
    char *processPath = getcwd(NULL, PATH_MAX);
    CSA(processPath == NULL, "readNFiles: getcwd", errno, ESOP("readNFiles", 0); UNLOCK(&(storage->globalMutex)))
    size_t pathLength = strlen(processPath); //il +1 è già compreso in MAX_PATH_LENGTH
    REALLOC(processPath, pathLength + 1) //getcwd alloca "MAX_PATH_LENGTH" bytes

    //cambio path [eliminare]
    DIR *directory = opendir(dirname);
    CSA(directory == NULL, "directory = opendir(dirname): ", errno, free(processPath); ESOP("readNFiles", 0); UNLOCK(&(storage->globalMutex))) //+1 per '\0'

    CSA(chdir(dirname) == -1, "readNFiles: chdir(dirname)", errno, free(processPath); UNLOCK(&(storage->globalMutex)); CLOSEDIR(directory))
    CSA(createReadNFiles(N, storage, clientFd) == -1, "readNFiles: createReadNFiles", errno, free(processPath); UNLOCK(&(storage->globalMutex)); ESOP("readNFiles", 0); CLOSEDIR(directory))
    CSA(chdir(processPath) == -1, "readNFiles: chdir(processPath)", errno, free(processPath); UNLOCK(&(storage->globalMutex)); ESOP("readNFiles", 0); CLOSEDIR(directory))

    free(processPath);
    CLOSEDIR(directory)

    ESOP("readNFiles", 1)
    UNLOCK(&(storage->globalMutex))
    //canWrite viene aggiornato in createReadNFiles

    return 0;
}

//funzione di supporto a readNFiles. Crea i file letti dal server e ne copia il contenuto
int createReadNFiles(int N, ServerStorage *storage, int clientFd)
{
    CS(storage == NULL, "readNFileServer: storage == NULL", EINVAL)
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    int NfileCreated = 0;
    int k;
    icl_entry_t *entry;
    char *key;
    File *serverFile;

    DIE(fprintf(storage->logFile, "File salvati: \n"))

    icl_hash_foreach(hashPtrF, k, entry, key, serverFile)
    {
            if (NfileCreated >= N) {
                return 0;
            }
            NfileCreated++;

            //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
            FILE *diskFile = fopen(basename(serverFile->path), "w");
            CS(diskFile == NULL, "createReadNFiles: fopen()", errno)

            //serverFile->fileContent è vuoto, allora il file creato riamen vuoto.
            if (serverFile->fileContent != NULL) {
                if (fputs(serverFile->fileContent, diskFile) == EOF) {
                    puts("createReadNFiles: fputs");
                    SYSCALL_NOTZERO(fclose(diskFile), "createReadNFiles: fclose() - termino processo")
                }
            }

            SYSCALL_NOTZERO(fclose(diskFile), "createReadNFiles: fclose() - termino processo")

            serverFile->canWriteFile = 0;

        DIE(fprintf(storage->logFile, "%s\n", serverFile->path))
    }

    return 0;
}

//manca buona parte della funzione - [controllare]
int writeFileServer(const char *pathname, const char *dirname, ServerStorage *storage, int clientFd) {
    LOCK(&(storage->globalMutex))
    writeLogFd_N_Date(storage->logFile, clientFd);

    //Argomenti invalidi
    CSA(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL, ESOP("writeFile", 0); UNLOCK(&(storage->globalMutex)))

    //Scrittura LOG
    if(dirname == NULL)
    {
        DIE(fprintf(storage->logFile, "Operazione: %s [dirname: %s]\n", "writeFile", "NULL"))
    }
    else
    {
        DIE(fprintf(storage->logFile, "Operazione: %s [dirname: %s]\n", "writeFile", dirname))
    }
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    CSA(storage == NULL, "writeFileServer: storage == NULL", EINVAL, ESOP("writeFile", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "writeFileServer: il file non è presente nella hash table", EPERM, ESOP("writeFile", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK

    IS_FILE_LOCKED(END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "writeFileServer: il file non è stato aperto da clientFd", EPERM, ESOP("writeFile", 0); END_WRITE_LOCK)
    //l'ultima operazione non era una open || il file non è vuoto
    CSA(serverFile->canWriteFile == 0 || serverFile->sizeFileByte != 0,
        "l'ultima operazione non era una open || il file non è vuoto", EPERM, ESOP("writeFile", 0); END_WRITE_LOCK)

    //ripristino "sizeFileByte" in caso di errore perché fillStructFile() non lo fa.
    CSA(fillStructFile(serverFile, storage, dirname) == -1, "", errno, serverFile->sizeFileByte = 0; ESOP("writeFile", 0); END_WRITE_LOCK)
    serverFile->canWriteFile = 0;

    DIE(fprintf(storage->logFile, "Numero byte scritti: %ld\n", serverFile->sizeFileByte))
    ESOP("writeFile", 1)

    END_WRITE_LOCK

    return 0;
}

//riempe i campi dello struct "File" a partire da file in disk con pathname: "pathname". Restituisce il dato "File"
int fillStructFile(File *serverFileF, ServerStorage *storage, const char *dirname)
{
    CS(storage == NULL, "fillStructFile: storage == NULL", EINVAL)
    CS(serverFileF == NULL, "fillStructFile: serverFileF == NULL", EINVAL)


    FILE *diskFile = fopen(serverFileF->path, "r");
    CS(diskFile == NULL, "fillStructFile: diskFile == NULL", errno)

    //-Inserimento sizeFileByte-
    rewind(diskFile);
    CSA(fseek(diskFile, 0L, SEEK_END) == -1, "fillStructFile: fseek", errno, FCLOSE(diskFile))

    size_t numCharFile = ftell(diskFile);
    CSA(numCharFile == -1, "fillStructFile: fseek", errno, FCLOSE(diskFile))
    CSA(storage->maxStorageBytes < numCharFile+1, "fillStructFile: spazio non sufficiente", ENOMEM, FCLOSE(diskFile))

    rewind(diskFile); //riposiziono il puntatore - [eliminare]
    serverFileF->sizeFileByte = numCharFile + 1; //memorizza anche il '\0' (perché anche esso occupa spazio)

    //lista che memorizza i file da copiare in dirname
    NodoQi_file *testaFilePtr = NULL;
    NodoQi_file *codaFilePtr = NULL;
    addBytes2Storage(serverFileF, storage, &testaFilePtr, &codaFilePtr);

    //-Inserimento fileContent-
    RETURN_NULL_SYSCALL(serverFileF->fileContent, malloc(sizeof(char) * serverFileF->sizeFileByte), "fillStructFile: malloc()")
    memset(serverFileF->fileContent, '\0', serverFileF->sizeFileByte);
    size_t readLength = fread(serverFileF->fileContent, 1, serverFileF->sizeFileByte - 1, diskFile); //size: 1 perché si legge 1 byte alla volta [eliminare]
    //Controllo errore fread
    CS(readLength != numCharFile || ferror(diskFile), "fillStructFile: fread", errno)

    FCLOSE(diskFile)

    if(testaFilePtr != NULL)
    {
        fprintf(storage->logFile, "File espulsi: \n");
        writePathToFile(testaFilePtr, storage->logFile);
    }

    if(dirname != NULL)
    {
        CS(copyFile2Dir(&testaFilePtr, &codaFilePtr, dirname) == -1, "appendToFileServer: copyFile2Dir", errno)
    }
    else
    {
        freeQueueFile(&testaFilePtr, &codaFilePtr);
    }

    return 0;
}

int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, ServerStorage *storage, int clientFd) {
    LOCK(&(storage->globalMutex))
    writeLogFd_N_Date(storage->logFile, clientFd);

    //Argomenti invalidi
    CSA(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))
    CSA(buf == NULL || strlen(buf) + 1 != size, "appendToFile: buf == NULL || strlen(buf) + 1", EINVAL, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex))) //"size": numero byte di buf e non il numero di caratteri


    //Scrittura LOG
    if(dirname == NULL)
    {
        DIE(fprintf(storage->logFile, "Operazione: %s [buffer: %s, dirname: %s]\n", "appendToFile", buf, "NULL"))
    }
    else
    {
        DIE(fprintf(storage->logFile, "Operazione: %s [buffer: %s, dirname: %s]\n", "appendToFile", buf, dirname))
    }
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    CSA(storage == NULL, "appendToFile: storage == NULL", EINVAL, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))
    CSA(storage->maxStorageBytes < size, "appendToFileServer: spazio non sufficiente", ENOMEM, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "il file non è presente nella hash table", EPERM, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK

    IS_FILE_LOCKED(END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "File non aperto da clientFd", EPERM, ESOP("appendToFile", 0); END_WRITE_LOCK)

    //lista che memorizza i file da copiare in dirname
    NodoQi_file *testaFilePtr = NULL;
    NodoQi_file *codaFilePtr = NULL;

    //=Inserimento buffer=
    if (serverFile->fileContent == NULL)
    {
        //il file è vuoto
        serverFile->sizeFileByte = size;
        addBytes2Storage(serverFile, storage, &testaFilePtr, &codaFilePtr);
        RETURN_NULL_SYSCALL(serverFile->fileContent, malloc(sizeof(char) * size), "fillStructFile: malloc()") //size conta anche il '\0'
        memset(serverFile->fileContent, '\0', size - 1);
        strncpy(serverFile->fileContent, buf, size);
    }
    else
    {
        serverFile->sizeFileByte = serverFile->sizeFileByte + size - 1;
        addBytes2Storage(serverFile, storage, &testaFilePtr, &codaFilePtr);
        REALLOC(serverFile->fileContent, sizeof(char) * serverFile->sizeFileByte) //il +1 è già contato in "serverFile->sizeFileByte" e in size (per questo motivo vien fatto -1)
        strncat(serverFile->fileContent, buf, size - 1); //sovvrascrive in automatico il '\0' di serverFile->fileContent e ne aggiunge uno alla fine
    }


    if(testaFilePtr != NULL)
    {
        fprintf(storage->logFile, "File espulsi: \n");
        writePathToFile(testaFilePtr, storage->logFile);
    }

    if(dirname != NULL)
    {
        DIE(fprintf(storage->logFile, "Numero byte scritti: %ld\n", size-1))
        CSA(copyFile2Dir(&testaFilePtr, &codaFilePtr, dirname) == -1, "appendToFileServer: copyFile2Dir", errno, ESOP("appendToFile", 0); END_WRITE_LOCK)
    }
    else
    {
        DIE(fprintf(storage->logFile, "Numero byte scritti: %ld\n", size))
        freeQueueFile(&testaFilePtr, &codaFilePtr);
    }

    serverFile->canWriteFile = 0;

    ESOP("AppendoToFile", 1)

    END_WRITE_LOCK

    return 0;
}

int lockFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname), "lockFileServer: pathname sbaglito", EINVAL, ESOP("lockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "lockFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    CSA(storage == NULL, "lockFileServer: storage == NULL", EINVAL, ESOP("lockFileServer", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "lockFileServer: file non presente nel server", EPERM, ESOP("lockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "lockFileServer: il file non è stato aperto da clientFd", EPERM, ESOP("lockFileServer", 0); END_WRITE_LOCK)

    setLockFile(serverFile, clientFd);

    serverFile->canWriteFile = 0;

    ESOP("lockFileServer", 1)

    END_WRITE_LOCK

    return 0;
}

int unlockFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname), "unlockFileServer: pathname sbaglito", EINVAL, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "unlockFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    CSA(storage == NULL, "unlockFileServer: storage == NULL", EINVAL, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "unlockFileServer: file non presente nel server", EPERM, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "unlockFileServer: il file non è stato aperto da clientFd", EPERM, ESOP("unlockFileServer", 0); END_WRITE_LOCK)

    CSA(serverFile->lockFd != clientFd, "unlockFileServer: non hai la lock", EPERM, ESOP("unlockFileServer", 0); END_WRITE_LOCK)

    setUnlockFile(serverFile);

    serverFile->canWriteFile = 0;

    ESOP("unlockFileServer", 1);

    END_WRITE_LOCK

    return 0;
}

int closeFileServer(const char *pathname, ServerStorage *storage, int clientFd) {
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL, ESOP("closeFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    CSA(storage == NULL, "closeFileServer: storage == NULL", EINVAL, ESOP("closeFileServer", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE close: file non presente nel server", EPERM, ESOP("closeFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK

    //se fd ha aperto il file, questo'ultimo viene chiuso
    CSA(deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1, "CLOSE: client non ha aperto il file", EPERM, ESOP("closeFileServer", 0); END_WRITE_LOCK)
    removeLock(serverFile, clientFd);

    serverFile->canWriteFile = 0;

    ESOP("closeFileServer", 1)

    END_WRITE_LOCK

    return 0;
}

int removeFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    LOCK(&(storage->globalMutex)) //NON posso usare la lock locale perché File viene eliminato

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname), "removeFileServer: pathname sbaglito", EINVAL, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    CSA(storage == NULL, "removeFileServer: storage == NULL", EINVAL, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE removeFileServer: file non presente nel server", EPERM, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)))

    IS_FILE_LOCKED(UNLOCK(&(storage->globalMutex)))


    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "removeFileServer: client non ha aperto il file",
        EPERM, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)))

    //elimino il singolo file
    size_t tempSizeBytes = serverFile->sizeFileByte;
    size_t lenPath = strlen(serverFile->path);
    char tempPath[lenPath + 1];
    strncpy(tempPath, serverFile->path, lenPath + 1);
    CSA(icl_hash_delete(hashPtrF, (void *) pathname, free, freeFileData) == -1, "removeFileServer: icl_hash_delete", EPERM, ESOP("removeFile", 0); END_WRITE_LOCK)

    //Aggiorno lo storage
    storage->currentStorageFiles--;
    storage->currentStorageBytes -= tempSizeBytes;
    deleteDataQueueStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), tempPath);

    ESOP("removeFile", 1);

    UNLOCK(&(storage->globalMutex))

    return 0;
}

//Rimuove il Fd e la lock del client quando questo chiude la connessione
int removeClientInfo(ServerStorage *storage, int clientFd)
{
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);
    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeConnection"))

    CSA(storage == NULL, "removeClientInfo: storage == NULL", EINVAL, ESOP("removeClientInfo", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    icl_hash_foreach(hashPtrF, k, entry, key, newFile)
    {
        removeLock(newFile, clientFd);

            //elimino l'fd dalla lista dei fd che hanno fatto la open
            deleteSortedList(&(newFile->fdOpen_SLPtr), clientFd);
    }

    ESOP("removeClientInfo", 1);

    UNLOCK(&(storage->globalMutex));
    return 0;
}

//Funzione di supporto che imposta la lock di un file
static void setLockFile(File *serverFile, int clientFd)
{
    if(serverFile->lockFd == -1)
        serverFile->lockFd = clientFd;
    else if(findDataQueue(serverFile->fdLock_TestaPtr, clientFd) == 0 && serverFile->lockFd != clientFd)
        push(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr), clientFd);
}

//Funzione di supporto che elimina la lock di un file
static void setUnlockFile(File *serverFile)
{
    if(serverFile->fdLock_TestaPtr == NULL)
        serverFile->lockFd = -1;
    else
        serverFile->lockFd = pop(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr));
}

//rimuove la lock di clientFd dalla lock in uso o dalla lista dei clientFd che sono in attesa di acquisire la lock
void removeLock(File *serverFile, int clientFd)
{
    if(serverFile->lockFd == clientFd)
        setUnlockFile(serverFile);
    else
        deleteDataQueue(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr), clientFd); //elimino clientId dalla coda se è presente
}

//elimina tutti i dati compresi in serverFile (ed elimina lo stesso serverFile)
void freeFileData(void *serverFile)
{
    File *serverFileF = serverFile;

    if (serverFileF->path != NULL) {
        free(serverFileF->path);
    }
    if (serverFileF->fileContent != NULL) {
        free(serverFileF->fileContent);
    }
    if (serverFileF->fdOpen_SLPtr != NULL) {
        freeSortedList(&(serverFileF->fdOpen_SLPtr));
    }
    if(serverFileF->fdLock_TestaPtr != NULL)
    {
        freeLista(&(serverFileF->fdLock_TestaPtr), &(serverFileF->fdLock_CodaPtr));
    }

    free(serverFileF);
}

void freeStorage(ServerStorage *storage)
{
    freeQueueStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr));
    icl_hash_destroy(storage->fileSystem, free, freeFileData); //da mettere controllo errore [controllare]

    free(storage);
}

void stampaHash(icl_hash_t *hashPtr) {
    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    printf("=====================stampaHash=====================\n");
    icl_hash_foreach(hashPtr, k, entry, key, newFile) {
            printf("-------%s-------\n", key);
            printf("Chiave: %s\n", key);
            printf("Path: %s\n", newFile->path);
//            printf("%d: cmp tra %s-%s\n", strcmp(key, newFile->path), key, newFile->path);
            printf("FIle name: %s\n", basename(newFile->path));
            printf("\nfileContent:\n%s\n\n", newFile->fileContent);
            printf("sizeFileByte: %ld\n", newFile->sizeFileByte);
            stampaSortedList(newFile->fdOpen_SLPtr);
            puts("");
            printf("Lock: %d\n", newFile->lockFd);
            stampa(newFile->fdLock_TestaPtr);

            //LOCK
//            puts("");
//            RwLock_t tempFileLock = newFile->fileLock;
//            printf("num_readers_active: %d\n", tempFileLock.num_readers_active);
//            printf("num_writers_waiting: %d\n", tempFileLock.num_writers_waiting);
//            printf("num_readers_active: %d\n", tempFileLock.writer_active);


            printf("--------------\n");
        }
    printf("=====================finestampaHash=====================\n");
}
