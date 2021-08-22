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
#include "../utils/conn.h"

static int setLockFile(File *serverFile, int clientFd);
static void setUnlockFile(File *serverFile);
void removeLock(File *serverFile, int clientFd);

//da finire [controllare]
int openFileServer(const char *pathname, int flags, ServerStorage *storage, int clientFd) {
    LOCK(&(storage->globalMutex))

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname) == -1, "openFile: pathname sbaglito", EINVAL, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s [flags: %d]\n", "openFile", flags))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    CSA(storage == NULL, "openFile: storage == NULL", EINVAL, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))
    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);

    //apre il file se questo esiste, altrimenti lo crea
    if(flags == O_CREATE + O_OPEN)
    {
        if(serverFile != NULL)
            flags = O_OPEN;
        else
            flags = O_CREATE;
    }

    //apre il file con la lock se questo esiste, altrimenti lo crea con la lock
    else if(flags == O_CREATE + O_OPEN + O_LOCK)
    {
        if(serverFile != NULL)
            flags = O_LOCK;
        else
            flags = O_CREATE + O_LOCK;
    }

    NodoQi_file *testaFilePtrF = NULL;
    NodoQi_file *codaFilePtrF = NULL;
    switch (flags) {
        case O_OPEN:
            while(storage->isRemovingFile == true)
            {
                puts("\n\n\n\n\n WRITE \n\n\n\n\n");
                WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
            }
            storage->isHandlingAPI = true;

            CSA(serverFile == NULL, "O_OPEN: File non presente", ENOENT, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK
            CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "O_OPEN: icl_hash_find", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)

            //se clientFd ha già aperto la lista, non si fa niente
            if(findSortedList(serverFile->fdOpen_SLPtr, clientFd) != 0)
            {
                insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
                serverFile->canWriteFile = 1;
            } else
            {
                PRINT("File già aperto")
            }

            storage->isHandlingAPI = false;
            SIGNAL(&(storage->condRemoveFile))
            END_WRITE_LOCK

            ESOP("OpenFile", 1)
            break;

        case O_CREATE:
            //devo usare la lock globale perché viene alterato lo storage
            CSA(serverFile != NULL, "O_CREATE: File già presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE: impossibile creare la entry nella hash", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

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

            UNLOCK(&(storage->globalMutex))
            break;

        case O_LOCK:
            while(storage->isRemovingFile == true)
            {
                puts("\n\n\n\n\n WRITE \n\n\n\n\n");
                WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
            }
            storage->isHandlingAPI = true;

            CSA(serverFile == NULL, "O_LOCK: File non presente", ENOENT, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            START_WRITE_LOCK
            CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "O_LOCK: icl_hash_find", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)

//            CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_LOCK: file già aperto dal client", EPERM, END_WRITE_LOCK)
            //Apro il file se questo non è presente
            if(findSortedList(serverFile->fdOpen_SLPtr, clientFd) != 0)
                insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
            int esito = setLockFile(serverFile, clientFd);
            serverFile->canWriteFile = 1;

            ESOP("OpenFile", 1)

            storage->isHandlingAPI = false;
            SIGNAL(&(storage->condRemoveFile))
            END_WRITE_LOCK

            return esito;
            break;

        case (O_CREATE + O_LOCK):
            CSA(serverFile != NULL, "O_CREATE + O_LOCK: File già presente", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE + O_LOCK: impossibile creare la entry nella hash", EPERM, ESOP("OpenFile", 0); UNLOCK(&(storage->globalMutex)))

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
            UNLOCK(&(storage->globalMutex))
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
    CSN(checkPathname(pathname) == -1, "createFile: pathname sbaglito", EINVAL)
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

//restituisce -1 in caso di errore, altrimenti il numero di byte letti
long readFileServer(const char *pathname, char **buf, size_t *size, ServerStorage *storage, int clientFd) {
    //in caso di errore, buf = NULL, size = 0
    assert(*buf == NULL);
    *size = 0;
    *buf = NULL;

    CSA(storage == NULL, "readFileServer: storage == NULL", EINVAL, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        puts("\n\n\n\n\n WRITE \n\n\n\n\n");
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname) == -1, "readFileServer: pathname sbaglito", EINVAL, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))
    DIE(fprintf(storage->logFile, "Operazione: %s\n", "readFile"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CSA(serverFile == NULL, "il file non è presente nella hash table", ENOENT, ESOP("readFile", 0); UNLOCK(&(storage->globalMutex)))

    START_READ_LOCK

    IS_FILE_LOCKED(END_READ_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "il file non è stato aperto da clientFd", EPERM, ESOP("readFile", 0); END_READ_LOCK)


    *size = serverFile->sizeFileByte;

    if(serverFile->fileContent != NULL)
    {
        RETURN_NULL_SYSCALL(*buf, malloc(*size), "readFileServer: *buf = malloc(*size)")
        memset(*buf, '\0', *size);
        strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte);
    }

    //readFile terminata con successo ==> non posso più fare la writeFile
    serverFile->canWriteFile = 0;

    if(*buf != NULL)
    {
        DIE(fprintf(storage->logFile, "Dim buffer inviato: %ld - Buffer: %s\n", *size, *buf))
    }
    else
    {
        DIE(fprintf(storage->logFile, "Dim buffer inviato: %ld - Buffer: %s\n", *size, "NULL"))
    }
    ESOP("readFile", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_READ_LOCK

    return *size;
}

//crea un file nella directory dirname e vi copia il contenuto di "pathname"
long copyFileToDirServer(const char *pathname, const char *dirname, ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "readFileServer: storage == NULL", EINVAL, ESOP("copyFileToDirServer", 0))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        puts("\n\n\n\n\n WRITE \n\n\n\n\n");
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname) == -1, "copyFileToDirServer: pathname sbaglito", EINVAL, ESOP("copyFileToDirServer", 0); UNLOCK(&(storage->globalMutex)))
    CSA(dirname == NULL, "copyFileToDirServer: dirname == NULL", EINVAL, ESOP("copyFileToDirServer", 0); UNLOCK(&(storage->globalMutex)))
    DIE(fprintf(storage->logFile, "Operazione: %s [dirname: %s]\n", "copyFileToDirServer", dirname))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CSA(serverFile == NULL, "il file non è presente nella hash table", ENOENT, ESOP("copyFileToDirServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK //non posso usare START_READ_LOCK perché uso "chdir"
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "il file non è presente nella hash table", ENOENT, ESOP("copyFileToDirServer", 0); UNLOCK(&(storage->globalMutex)))

    IS_FILE_LOCKED(END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "il file non è stato aperto da clientFd", EPERM, ESOP("copyFileToDirServer", 0); END_WRITE_LOCK)

    //Obbligatorio la mutex globale perché si usa chdir
    LOCK(&(storage->globalMutex))
    CSA(copyFileToDirHandler(serverFile, dirname), "copyFileToDirHandler", errno, ESOP("copyFileToDirServer", 0); END_WRITE_LOCK; UNLOCK(&(storage->globalMutex)))
    UNLOCK(&(storage->globalMutex))

    serverFile->canWriteFile = 0;
    ESOP("copyFileToDirServer", 1)

    long returnValue = serverFile->sizeFileByte;

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK
//    UNLOCK(&(storage->globalMutex))

    return returnValue;
}

//funzione di supporto per copyFileToDirServer
static int copyFileToDirHandler(File *serverFile, const char dirname[])
{
    //==Calcola il path dove lavora il processo==
    char *processPath = getcwd(NULL, PATH_MAX);
    CS(processPath == NULL, "getcwd", errno)
    size_t pathLength = strlen(processPath); //il +1 è già compreso in MAX_PATH_LENGTH
    REALLOC(processPath, pathLength + 1) //getcwd alloca "MAX_PATH_LENGTH" bytes

    //cambio path [eliminare]
    DIR *directory = opendir(dirname);
    CSA(directory == NULL, "opensssdir(dirname): ", errno, free(processPath)) //+1 per '\0'

    CSA(chdir(dirname) == -1, "chdir(dirname)", errno, free(processPath))
    if(creatFileAndCopy(serverFile) == -1)
    {
        free(processPath);
        CLOSEDIR(directory)
        SYSCALL(chdir(processPath), "chdir(processPath)") //Non può fallire: lascerebbe il file system in uno stato incosistente
    }
    SYSCALL(chdir(processPath), "chdir(processPath)") //Non può fallire: lascerebbe il file system in uno stato incosistente

    free(processPath);
    CLOSEDIR(directory)

    return 0;
}

int readNFilesServer(int N, const char *dirname, ServerStorage *storage, int clientFd, unsigned long long int *bytesLetti)
{
    *bytesLetti = 0;
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        puts("\n\n\n\n\n WRITE \n\n\n\n\n");
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

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
    int returnValue = createReadNFiles(N, storage, clientFd, bytesLetti);
    CSA(returnValue == -1, "readNFiles: createReadNFiles", errno, free(processPath); UNLOCK(&(storage->globalMutex)); ESOP("readNFiles", 0); CLOSEDIR(directory))
    CSA(chdir(processPath) == -1, "readNFiles: chdir(processPath)", errno, free(processPath); UNLOCK(&(storage->globalMutex)); ESOP("readNFiles", 0); CLOSEDIR(directory))

    free(processPath);
    CLOSEDIR(directory)

    ESOP("readNFiles", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    UNLOCK(&(storage->globalMutex))
    //canWrite viene aggiornato in createReadNFiles

    return returnValue;
}

//funzione di supporto a readNFiles. Crea i file letti dal server e ne copia il contenuto
int createReadNFiles(int N, ServerStorage *storage, int clientFd, unsigned long long int *bytesLetti)
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
                return NfileCreated;
            }

            if(creatFileAndCopy(serverFile) == 0)
            {
                NfileCreated++;
                *bytesLetti += serverFile->sizeFileByte;
            }

            serverFile->canWriteFile = 0;

        DIE(fprintf(storage->logFile, "%s\n", serverFile->path))
    }

    return NfileCreated;
}

//funzione di supporto per createReadNFiles copyFileToDirServer
//crea un file e vi coppia il contenuto presente in "serverFile"
//ritorna -1 in caso di errore, 0 altrimenti

static int creatFileAndCopy(File *serverFile)
{
    //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
    FILE *diskFile = fopen(basename(serverFile->path), "w");
    CS(diskFile == NULL, "creatFileAndCopy: fopen()", errno)

    //serverFile->fileContent è vuoto, allora il file creato riamen vuoto.
    if (serverFile->fileContent != NULL) {
        if (fputs(serverFile->fileContent, diskFile) == EOF) {
            puts("creatFileAndCopy: fputs");
            SYSCALL_NOTZERO(fclose(diskFile), "creatFileAndCopy: fclose() - termino processo")
        }
    }

    SYSCALL_NOTZERO(fclose(diskFile), "creatFileAndCopy: fclose() - termino processo")

    return 0;
}

//restituisce -1 in caso di errore, altrimenti il numero di byte scritti
long writeFileServer(const char *pathname, const char *dirname, ServerStorage *storage, int clientFd) {
    CSA(storage == NULL, "writeFileServer: storage == NULL", EINVAL, ESOP("writeFile", 0))
    LOCK(&(storage->globalMutex))

    while(storage->isRemovingFile == true)
    {
        puts("\n\n\n\n\n WRITE \n\n\n\n\n");
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;


    writeLogFd_N_Date(storage->logFile, clientFd);

    //Argomenti invalidi
    CSA(checkPathname(pathname) == -1, "openFile: pathname sbaglito", EINVAL, ESOP("writeFile", 0); UNLOCK(&(storage->globalMutex)))

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

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "writeFileServer: il file non è presente nella hash table", ENOENT, ESOP("writeFile", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK
    //Il file potrebbe essere stato eliminato dalla cache
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "\n\n\n\n\n\n\n\n\n  WRITE writeFileServer: icl_hash_find\n\n\n\n\n\n\n\n\n", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)

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

    long returnValue = serverFile->sizeFileByte; //garantisce la mutua esclusione

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK

    return returnValue;
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


    //Il file è vuoto
    if(numCharFile == 0)
    {
        serverFileF->sizeFileByte;
        FCLOSE(diskFile)

        return 0;
    }

    serverFileF->sizeFileByte = numCharFile + 1; //memorizza anche il '\0' (perché anche esso occupa spazio)

    //lista che memorizza i file da copiare in dirname
    NodoQi_file *testaFilePtr = NULL;
    NodoQi_file *codaFilePtr = NULL;

    LOCK(&(storage->globalMutex)) //serve la lock globale perché si accede ai dati dello storage
    addBytes2Storage(serverFileF, storage, &testaFilePtr, &codaFilePtr);
    UNLOCK(&(storage->globalMutex)) //serve la lock perché si usa chdir

    //-Inserimento fileContent-
    RETURN_NULL_SYSCALL(serverFileF->fileContent, malloc(sizeof(char) * serverFileF->sizeFileByte), "fillStructFile: malloc()")
    memset(serverFileF->fileContent, '\0', serverFileF->sizeFileByte);
    size_t readLength = fread(serverFileF->fileContent, 1, serverFileF->sizeFileByte - 1, diskFile); //size: 1 perché si legge 1 byte alla volta [eliminare]
    //Controllo errore fread
    CS(readLength != numCharFile || ferror(diskFile), "fillStructFile: fread", errno)

    FCLOSE(diskFile)

    LOCK(&(storage->globalMutex)) //serve la lock perché si usa chdir e la cosa per la cache
    if(testaFilePtr != NULL)
    {
        fprintf(storage->logFile, "File espulsi: \n");
        writePathToFile(testaFilePtr, storage->logFile);

        if (dirname != NULL)
        {
            CS(copyFile2Dir(&testaFilePtr, &codaFilePtr, dirname) == -1, "fillStructFile: copyFile2Dir", errno)
            if(testaFilePtr != NULL) {
                freeQueueFile(&testaFilePtr, &codaFilePtr);
            }
        } else
        {
            freeQueueFile(&testaFilePtr, &codaFilePtr);
        }
    }
    UNLOCK(&(storage->globalMutex))

    return 0;
}

int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, ServerStorage *storage, int clientFd) {
    CSA(storage == NULL, "appendToFile: storage == NULL", EINVAL, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))
    LOCK(&(storage->globalMutex))

    while(storage->isRemovingFile == true)
    {
        puts("\n\n\n\n\n WRITE \n\n\n\n\n");
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    //Argomenti invalidi
    CSA(checkPathname(pathname) == -1, "openFile: pathname sbaglito", EINVAL, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))
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


    CSA(storage->maxStorageBytes < size, "appendToFileServer: spazio non sufficiente", ENOMEM, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "il file non è presente nella hash table", ENOENT, ESOP("appendToFile", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK
    //Il file potrebbe essere stato eliminato dalla cache
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "appendToFile: icl_hash_find", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)

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

        LOCK(&(storage->globalMutex)) //serve la lock globale perché si accede ai dati dello storage
        addBytes2Storage(serverFile, storage, &testaFilePtr, &codaFilePtr);
        UNLOCK(&(storage->globalMutex))

        RETURN_NULL_SYSCALL(serverFile->fileContent, malloc(sizeof(char) * size), "fillStructFile: malloc()") //size conta anche il '\0'
        memset(serverFile->fileContent, '\0', size);
        strncpy(serverFile->fileContent, buf, size);
    }
    else
    {
        serverFile->sizeFileByte = serverFile->sizeFileByte + size - 1;
        CSA(storage->maxStorageBytes < serverFile->sizeFileByte, "appendToFileServer: spazio non sufficiente", ENOMEM, ESOP("appendToFile", 0); END_WRITE_LOCK)

        LOCK(&(storage->globalMutex)) //serve la lock globale perché si accede ai dati dello storage
        addBytes2Storage(serverFile, storage, &testaFilePtr, &codaFilePtr);
        UNLOCK(&(storage->globalMutex))

        REALLOC(serverFile->fileContent, sizeof(char) * serverFile->sizeFileByte) //il +1 è già contato in "serverFile->sizeFileByte" e in size (per questo motivo vien fatto -1)
        strncat(serverFile->fileContent, buf, size - 1); //sovvrascrive in automatico il '\0' di serverFile->fileContent e ne aggiunge uno alla fine
    }


    LOCK(&(storage->globalMutex)) //serve la lock globale perché si accede ai dati dello storage
    if(testaFilePtr != NULL)
    {
        fprintf(storage->logFile, "File espulsi: \n");
        writePathToFile(testaFilePtr, storage->logFile);

        if (dirname != NULL) {
            DIE(fprintf(storage->logFile, "Numero byte scritti: %ld\n", size - 1))
            CSA(copyFile2Dir(&testaFilePtr, &codaFilePtr, dirname) == -1, "appendToFileServer: copyFile2Dir", errno, ESOP("appendToFile", 0); END_WRITE_LOCK)
        } else {
            DIE(fprintf(storage->logFile, "Numero byte scritti: %ld\n", size))
            freeQueueFile(&testaFilePtr, &codaFilePtr);
        }
    }
    UNLOCK(&(storage->globalMutex)) //serve la lock globale perché si accede ai dati dello storage

    serverFile->canWriteFile = 0;

    ESOP("AppendoToFile", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK

    return 0;
}

//ritorna 0 se clinetFd ha acquisito la lock
//-2 se è in coda
//-1 se ha fallito
int lockFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "lockFileServer: storage == NULL", EINVAL, ESOP("lockFileServer", 0))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname) == -1, "lockFileServer: pathname sbaglito", EINVAL, ESOP("lockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "lockFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "lockFileServer: file non presente nel server", ENOENT, ESOP("lockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK
    //Il file potrebbe essere stato eliminato dalla cache
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "lockFileServer: icl_hash_find", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)


    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "lockFileServer: il file non è stato aperto da clientFd", EPERM, ESOP("lockFileServer", 0); END_WRITE_LOCK)

    int esito = setLockFile(serverFile, clientFd);

    serverFile->canWriteFile = 0;

    ESOP("lockFileServer", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK

    return esito;
}

int unlockFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "unlockFileServer: storage == NULL", EINVAL, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname) == -1, "unlockFileServer: pathname sbaglito", EINVAL, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "unlockFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "unlockFileServer: file non presente nel server", ENOENT, ESOP("unlockFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK
    //Il file potrebbe essere stato eliminato dalla cache
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "unlockFileServer: icl_hash_find", ENOENT, ESOP("closeFileServer", 0); END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "unlockFileServer: il file non è stato aperto da clientFd", EPERM, ESOP("unlockFileServer", 0); END_WRITE_LOCK)

    CSA(serverFile->lockFd != clientFd, "unlockFileServer: non hai la lock", EPERM, ESOP("unlockFileServer", 0); END_WRITE_LOCK)

    setUnlockFile(serverFile);

    serverFile->canWriteFile = 0;

    ESOP("unlockFileServer", 1);

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK

    return 0;
}

int closeFileServer(const char *pathname, ServerStorage *storage, int clientFd) {
    CSA(storage == NULL, "closeFileServer: storage == NULL", EINVAL, ESOP("closeFileServer", 0))

    LOCK(&(storage->globalMutex))

    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }

    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname) == -1, "closeFileServer: pathname sbaglito", EINVAL, ESOP("closeFileServer", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE close: file non presente nel server", ENOENT, ESOP("closeFileServer", 0); UNLOCK(&(storage->globalMutex)))

    START_WRITE_LOCK
    //Il file potrebbe essere stato eliminato dalla cache
    CSA(icl_hash_find(hashPtrF, (void *) pathname) == NULL, "\n\n\n\n\n\n\n\n\n  CLOSE closeFileServer: icl_hash_find\n\n\n\n\n\n\n\n\n", ENOENT, ESOP("closeFileServer", 0); storage->isHandlingAPI = false; END_WRITE_LOCK)


    //se fd ha aperto il file, questo'ultimo viene chiuso
    CSA(deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1, "CLOSE: client non ha aperto il file", EPERM, ESOP("closeFileServer", 0); END_WRITE_LOCK)
    removeLock(serverFile, clientFd);

    serverFile->canWriteFile = 0;

    ESOP("closeFileServer", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_WRITE_LOCK

    return 0;
}

int removeFileServer(const char *pathname, ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "removeFileServer: storage == NULL", EINVAL, ESOP("removeFile", 0))
    LOCK(&(storage->globalMutex))

    while (storage->isHandlingAPI == true)
    {
        puts("\n\n\n\n\n REMOVE \n\n\n\n\n");

        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }

    storage->isRemovingFile = true;
    writeLogFd_N_Date(storage->logFile, clientFd);

    CSA(checkPathname(pathname) == -1, "removeFileServer: pathname sbaglito", EINVAL, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)))

    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeFileServer"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE removeFileServer: file non presente nel server", ENOENT, ESOP("removeFile", 0); SIGNAL(&(storage->condRemoveFile)); UNLOCK(&(storage->globalMutex)))

//    //Non rilascio storage->globalMutex, altrimenti qualcuno rischia di accedere ad un file eliminato
//    //è necessario eseguire rwLock_startWriting, perché removeFile potrebbe essere chiamata mentre un'altra funzione è in esecuzione
//    // (in questo modo mi metto in attesa che questa finisca)
//        LOCK(&(serverFile->fileLock.mutexFile))
//        rwLock_startWriting(&serverFile->fileLock);
//        UNLOCK(&(serverFile->fileLock.mutexFile))

    IS_FILE_LOCKED(SIGNAL(&(storage->condRemoveFile)); UNLOCK(&(storage->globalMutex)))

    CSA(serverFile->lockFd != clientFd, "removeFileServer: clientFd non ha la lock", EPERM, ESOP("removeFile", 0); SIGNAL(&(storage->condRemoveFile)); UNLOCK(&(storage->globalMutex)))

    assert(findSortedList(serverFile->fdOpen_SLPtr, clientFd) != -1); //se possiede la lock, allora ha sicuramento aperto il file

//    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "removeFileServer: client non ha aperto il file", EPERM, ESOP("removeFile", 0); UNLOCK(&(storage->globalMutex)));


    //Notifico i file che hanno fatto la lock
    while(serverFile->fdLock_TestaPtr != NULL)
    {
        int fdClinetLock = pop(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr));
        int esito = ENOENT;
        WRITEN_C(fdClinetLock, &esito, sizeof(int), "setUnlockFile: wrporcoooiten()") //notifico il client che ha ottenuto la lock
    }

    //elimino il singolo file
    size_t tempSizeBytes = serverFile->sizeFileByte;
    size_t lenPath = strlen(serverFile->path);
    char tempPath[lenPath + 1];
    strncpy(tempPath, serverFile->path, lenPath + 1);
    CSA(icl_hash_delete(hashPtrF, (void *) pathname, free, freeFileData) == -1, "removeFileServer: icl_hash_delete", EPERM, ESOP("removeFile", 0); SIGNAL(&(storage->condRemoveFile)); UNLOCK(&(storage->globalMutex)))


    //Aggiorno lo storage
    storage->currentStorageFiles--;
    storage->currentStorageBytes -= tempSizeBytes;
    deleteDataQueueStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), tempPath);

    ESOP("removeFile", 1);

    //Non viene chiamata END_WRITE_LOCK perché la lock non esiste più
    storage->isRemovingFile = false;
    SIGNAL(&(storage->condRemoveFile))
    UNLOCK(&(storage->globalMutex))

    return 0;
}

//restituisce:
// -1 in caso di errore
// 0 se il file "pathname" non è presente in hashPtrF
//1 se clientfd NON ha aperto il file "pathname"
//2 se clientfd ha aperto il file "pathname"
int isPathPresentServer(const char pathname[], ServerStorage *storage, int clientFd)
{
    CS(checkPathname(pathname) == -1, "isPathPresentServer: pathname sbaglito", EINVAL)

    CSA(storage == NULL, "isPathPresentServer: storage == NULL", EINVAL, UNLOCK(&(storage->globalMutex)))
    LOCK(&(storage->globalMutex)) //NON posso usare la lock locale perché File viene eliminato
    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);


    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    if(serverFile == NULL)
    {
        UNLOCK(&(storage->globalMutex))
        return NOT_PRESENTE;
    }
    START_READ_LOCK

    if(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1)
    {
        storage->isHandlingAPI = false;
        SIGNAL(&(storage->condRemoveFile))
        END_READ_LOCK
        return NOT_APERTO;
    }
    else
    {
        storage->isHandlingAPI = false;
        SIGNAL(&(storage->condRemoveFile))
        END_READ_LOCK
        return APERTO;
    }

}

//restituisce SizeFileByte, altrimenti -1 in caso di errore
size_t getSizeFileByteServer(const char pathname[], ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "getSizeFileByte: storage == NULL", EINVAL, ESOP("getSizeFileByte", 0))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);

    //argomenti invalidi
    CSA(checkPathname(pathname) == -1, "getSizeFileByte: pathname sbaglito", EINVAL, ESOP("getSizeFileByte", 0); UNLOCK(&(storage->globalMutex)))
    DIE(fprintf(storage->logFile, "Operazione: %s\n", "getSize"))
    DIE(fprintf(storage->logFile, "Pathname: %s\n", pathname))


    icl_hash_t *hashPtrF = storage->fileSystem;
    assert(hashPtrF != NULL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CSA(serverFile == NULL, "il file non è presente nella hash table", ENOENT, ESOP("getSizeFileByte", 0); UNLOCK(&(storage->globalMutex)))

    START_READ_LOCK

    IS_FILE_LOCKED(END_READ_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "il file non è stato aperto da clientFd", EPERM, ESOP("getSizeFileByte", 0); END_READ_LOCK)

    size_t sizeTemp = serverFile->sizeFileByte;

    ESOP("getSizeFileByte", 1)

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    END_READ_LOCK

    return sizeTemp;
}

//Rimuove il Fd e la lock del client quando questo chiude la connessione
int removeClientInfo(ServerStorage *storage, int clientFd)
{
    CSA(storage == NULL, "removeClientInfo: storage == NULL", EINVAL, ESOP("removeClientInfo", 0))
    LOCK(&(storage->globalMutex))
    while(storage->isRemovingFile == true)
    {
        WAIT(&(storage->condRemoveFile), &(storage->globalMutex))
    }
    storage->isHandlingAPI = true;

    writeLogFd_N_Date(storage->logFile, clientFd);
    DIE(fprintf(storage->logFile, "Operazione: %s\n", "closeConnection"))

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

    storage->isHandlingAPI = false;
    SIGNAL(&(storage->condRemoveFile))
    UNLOCK(&(storage->globalMutex));
    return 0;
}

//Funzione di supporto che imposta la lock di un file
//ritorna 1 se ha impostato la lock, -2 se il client è stato inserito in coda
static int setLockFile(File *serverFile, int clientFd) {
    if (serverFile->lockFd == -1)
    {
        serverFile->lockFd = clientFd;
        return 1;
    }
    else if(findDataQueue(serverFile->fdLock_TestaPtr, clientFd) == 0 && serverFile->lockFd != clientFd)
    {
        push(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr), clientFd);
        return -2;
    }
}

//Funzione di supporto che elimina la lock di un file
static void setUnlockFile(File *serverFile)
{
    if(serverFile->fdLock_TestaPtr == NULL)
        serverFile->lockFd = -1;
    else
    {
        serverFile->lockFd = pop(&(serverFile->fdLock_TestaPtr), &(serverFile->fdLock_CodaPtr));
        int esito = 0;
        WRITEN_C(serverFile->lockFd, &esito, sizeof(int), "setUnlockFile: wrporcoooiten()") //notifico il client che ha ottenuto la lock
    }
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

void stampaHash(ServerStorage *storage)
{
    assert(storage != NULL);
    LOCK(&(storage->globalMutex))

    icl_hash_t *hashPtr = storage->fileSystem;
    assert(hashPtr != NULL);

    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    printf("=====================stampaHash=====================\n");
    icl_hash_foreach(hashPtr, k, entry, key, newFile) {
            printf("-------%s-------\n", key);
//            printf("Chiave: %s\n", key);
//            printf("Path: %s\n", newFile->path);
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
    UNLOCK(&(storage->globalMutex))
}
