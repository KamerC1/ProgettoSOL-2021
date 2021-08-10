#include <stdio.h>
#include <dirent.h>
#include <sys/unistd.h>

#include "../include/serverAPI.h"
#include "../utils/util.h"
#include "../utils/utilsPathname.c"

static void setLockFile(File *serverFile, int clientFd);
static void setUnlockFile(File *serverFile);

//da finire [controllare]
int openFileServer(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd) {
    //argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)

    LOCK(&globalMutex)
    CS(hashPtrF == NULL, "openFile: hashPtrF == NULL", EINVAL)
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);

    switch (flags) {
        case O_OPEN:
            CSA(serverFile == NULL, "O_OPEN: File non presente", EPERM, UNLOCK(&globalMutex))

            START_WRITE_LOCK

            //clientFd ha già aperto la lista
            CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_OPEN: file già aperto dal client", EPERM, END_WRITE_LOCK)

            insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
            serverFile->canWriteFile = 1;

            END_WRITE_LOCK

            break;

        case O_CREATE:
            CSA(serverFile != NULL, "O_CREATE: File già presente", EPERM, UNLOCK(&globalMutex))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE: impossibile creare la entry nella hash", EPERM, UNLOCK(&globalMutex))

            START_WRITE_LOCK

            CSA(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile) == NULL,
                "O_CREATE: impossibile inserire File nella hash", EPERM, END_WRITE_LOCK);

            serverFile->canWriteFile = 1;

            END_WRITE_LOCK
            break;

        case O_LOCK:
            CSA(serverFile == NULL, "O_LOCK: File non presente", EPERM, UNLOCK(&globalMutex))

            START_WRITE_LOCK

            CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_LOCK: file già aperto dal client", EPERM, END_WRITE_LOCK)
            insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);
            setLockFile(serverFile, clientFd);
            serverFile->canWriteFile = 1;

            END_WRITE_LOCK
            break;

        case (O_CREATE + O_LOCK):
            CSA(serverFile != NULL, "O_CREATE + O_LOCK: File già presente", EPERM, UNLOCK(&globalMutex))

            serverFile = createFile(pathname, clientFd);
            CSA(serverFile == NULL, "O_CREATE + O_LOCK: impossibile creare la entry nella hash", EPERM, UNLOCK(&globalMutex))

            START_WRITE_LOCK

            CSA(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile) == NULL,
               "O_CREATE + O_LOCK: impossibile inserire File nella hash", EPERM, END_WRITE_LOCK)
            serverFile->lockFd = clientFd;
            serverFile->canWriteFile = 1;

            END_WRITE_LOCK
            break;

        default:
            //la flag non corrisponde
            PRINT("flag sbagliata")

            errno = EINVAL;
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

int readFileServer(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd) {
    //in caso di errore, buf = NULL, size = 0
    *size = 0;
    *buf = NULL;
    //argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)

    LOCK(&globalMutex)
    CSA(hashPtrF == NULL, "openFile: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex))

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CSA(serverFile == NULL, "il file non è presente nella hash table", EPERM, UNLOCK(&globalMutex))

    START_READ_LOCK

    IS_FILE_LOCKED(END_READ_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "il file non è stato aperto da clientFd", EPERM, END_READ_LOCK)


    *size = serverFile->sizeFileByte;
    RETURN_NULL_SYSCALL(*buf, malloc(*size), "readFile: *buf = malloc(*size)")
    memset(*buf, '\0', *size);

    strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte);

    //readFile terminata con successo ==> non posso più fare la writeFile
    serverFile->canWriteFile = 0;

    END_READ_LOCK

    return 0;
}

int readNFilesServer(int N, const char *dirname, icl_hash_t *hashPtrF) {
    CS(dirname == NULL, "readNFiles: dirname == NULL", EINVAL)

    //imposta il numero di file da leggere
    if (N <= 0 || hashPtrF->nentries < N)
        N = hashPtrF->nentries;

    //==Calcola il path dove lavora il processo==
    //calcolo la lunghezza del path [eliminare]
    char *temp = getcwd(NULL, MAX_PATH_LENGTH);
    CS(temp == NULL, "readNFiles: getcwd", errno)
    int pathLength = strlen(temp); //il +1 è già compreso in MAX_PATH_LENGTH
    free(temp);
    char processPath[pathLength + 1]; //+1 per '\0'
    CS(getcwd(processPath, pathLength + 1) == NULL, "readNFiles: ", errno) //+1 per '\0'

    //cambio path [eliminare]
    DIR *directory = opendir(dirname);
    CS(directory == NULL, "directory = opendir(dirname): ", errno) //+1 per '\0'

    //spiegazione su notion [eliminare]
    LOCK(&globalMutex)
    CSA(hashPtrF == NULL, "readNFilesServer: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex); CLOSEDIR(directory))
    CSA(chdir(dirname) == -1, "readNFiles: chdir(dirname)", errno, UNLOCK(&globalMutex); CLOSEDIR(directory))
    CSA(createReadNFiles(N, hashPtrF) == -1, "readNFiles: createReadNFiles", errno, UNLOCK(&globalMutex); CLOSEDIR(directory))
    CSA(chdir(processPath) == -1, "readNFiles: chdir(processPath)", errno, UNLOCK(&globalMutex); CLOSEDIR(directory))
    UNLOCK(&globalMutex)

    CLOSEDIR(directory)

    //canWrite viene aggiornato in createReadNFiles

    return 0;
}

//funzione di supporto a readNFiles. Crea i file letti dal server e ne copia il contenuto
int createReadNFiles(int N, icl_hash_t *hashPtrF) {
    int NfileCreated = 0;
    int k;
    icl_entry_t *entry;
    char *key;
    File *serverFile;

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
    }

    return 0;
}

//manca buona parte della funzione - [controllare]
int writeFileServer(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd) {
    //Argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)

    LOCK(&globalMutex)
    CSA(hashPtrF == NULL, "writeFileServer: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex))

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "writeFileServer: il file non è presente nella hash table", EPERM, UNLOCK(&globalMutex))

    START_WRITE_LOCK

    IS_FILE_LOCKED(END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "writeFileServer: il file non è stato aperto da clientFd", EPERM, END_WRITE_LOCK)
    //l'ultima operazione non era una open || il file non è vuoto
    CSA(serverFile->canWriteFile == 0 || serverFile->sizeFileByte != 0,
        "l'ultima operazione non era una open || il file non è vuoto", EPERM, END_WRITE_LOCK)

    //ripristino "sizeFileByte" in caso di errore perché fillStructFile() non lo fa.
    CSA(fillStructFile(serverFile) == -1, "", errno,
        serverFile->sizeFileByte = 0; END_WRITE_LOCK)
    serverFile->canWriteFile = 0;

    END_WRITE_LOCK

    return 0;
}

//riempe i campi dello struct "File" a partire da file in disk con pathname: "pathname". Restituisce il dato "File"
int fillStructFile(File *serverFileF) {
    CS(serverFileF == NULL, "fillStructFile: serverFileF == NULL", EINVAL)


    FILE *diskFile = fopen(serverFileF->path, "r");
    CS(diskFile == NULL, "fillStructFile: diskFile == NULL", errno)

    //-Inserimento sizeFileByte-
    rewind(diskFile);
    CSA(fseek(diskFile, 0L, SEEK_END) == -1, "fillStructFile: fseek", errno, FCLOSE(diskFile))

    serverFileF->sizeFileByte = ftell(diskFile);
    CSA(serverFileF->sizeFileByte == -1, "fillStructFile: fseek", errno, FCLOSE(diskFile))

    rewind(diskFile); //riposiziono il puntatore - [eliminare]

    size_t numCharFile = serverFileF->sizeFileByte; //numero di caratteri presente nel file
    serverFileF->sizeFileByte += 1; //memorizza anche il '\0' (perché anche esso occupa spazio)


    //-Inserimento fileContent-
    RETURN_NULL_SYSCALL(serverFileF->fileContent, malloc(sizeof(char) * numCharFile + 1), "fillStructFile: malloc()")
    memset(serverFileF->fileContent, '\0', numCharFile + 1);
    size_t readLength = fread(serverFileF->fileContent, 1, numCharFile,
                              diskFile); //size: 1 perché si legge 1 byte alla volta [eliminare]
    //Controllo errore fread
    CS(readLength != numCharFile || ferror(diskFile), "fillStructFile: fread", errno)

    FCLOSE(diskFile)
    return 0;
}

int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd) {
    //Argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)
    CS(buf == NULL || strlen(buf) + 1 != size, "appendToFile: buf == NULL || strlen(buf) + 1",
       EINVAL) //"size": numero byte di buf e non il numero di caratteri

    LOCK(&globalMutex)

    CSA(hashPtrF == NULL, "appendToFile: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex))
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "il file non è presente nella hash table", EPERM, UNLOCK(&globalMutex))

    START_WRITE_LOCK

    IS_FILE_LOCKED(END_WRITE_LOCK)

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "File non aperto da clientFd", EPERM, END_WRITE_LOCK)
    //=Inserimento buffer=
    if (serverFile->fileContent == NULL)
    {
        //il file è vuoto
        RETURN_NULL_SYSCALL(serverFile->fileContent, malloc(sizeof(char) * size), "fillStructFile: malloc()") //size conta anche il '\0'
        memset(serverFile->fileContent, '\0', size - 1);
        strncpy(serverFile->fileContent, buf, size);
        serverFile->sizeFileByte = size;
    }
    else
    {
        REALLOC(serverFile->fileContent, sizeof(char) * (serverFile->sizeFileByte + size - 1)) //il +1 è già contato in "serverFile->sizeFileByte" e in size (per questo motivo vien fatto -1)
        CSA(strncat(serverFile->fileContent, buf, size - 1) == NULL, "appendToFile: strncat", errno, END_WRITE_LOCK) //sovvrascrive in automatico il '\0' di serverFile->fileContent e ne aggiunge uno alla fine
        serverFile->sizeFileByte = serverFile->sizeFileByte + size - 1;
    }

    serverFile->canWriteFile = 0;

    END_WRITE_LOCK

    return 0;
}

int lockFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd)
{
    CS(checkPathname(pathname), "lockFileServer: pathname sbaglito", EINVAL)

    LOCK(&globalMutex);

    CSA(hashPtrF == NULL, "lockFileServer: pathname == NULL", EINVAL, UNLOCK(&globalMutex))
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "lockFileServer: file non presente nel server", EPERM, UNLOCK(&globalMutex))

    START_WRITE_LOCK

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "lockFileServer: il file non è stato aperto da clientFd", EPERM, END_WRITE_LOCK)

    setLockFile(serverFile, clientFd);

    serverFile->canWriteFile = 0;


    END_WRITE_LOCK

    return 0;
}

int unlockFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd)
{
    CS(checkPathname(pathname), "unlockFileServer: pathname sbaglito", EINVAL)

    LOCK(&globalMutex);

    CSA(hashPtrF == NULL, "unlockFileServer: pathname == NULL", EINVAL, UNLOCK(&globalMutex))
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "unlockFileServer: file non presente nel server", EPERM, UNLOCK(&globalMutex))

    START_WRITE_LOCK

    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
        "unlockFileServer: il file non è stato aperto da clientFd", EPERM, END_WRITE_LOCK)

    CSA(serverFile->lockFd != clientFd, "unlockFileServer: non hai la lock", EPERM, END_WRITE_LOCK)

    setUnlockFile(serverFile);

    serverFile->canWriteFile = 0;


    END_WRITE_LOCK

    return 0;
}

int closeFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd) {
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)

    LOCK(&globalMutex);

    CSA(hashPtrF == NULL, "ERRORE close: pathname == NULL", EINVAL, UNLOCK(&globalMutex))
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE close: file non presente nel server", EPERM, UNLOCK(&globalMutex))

    START_WRITE_LOCK

    //se fd ha aperto il file, questo'ultimo viene chiuso
    CSA(deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1, "CLOSE: client non ha aperto il file", EPERM, END_WRITE_LOCK)
    serverFile->canWriteFile = 0;

    END_WRITE_LOCK

    return 0;
}

//manca la gestione della lock [eliminare]
int removeFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd) {
    CS(checkPathname(pathname), "removeFileServer: pathname sbaglito", EINVAL)

    LOCK(&globalMutex) //NON posso usare la lock locale perché File viene eliminato

    CSA(hashPtrF == NULL, "removeFileServer: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex))
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CSA(serverFile == NULL, "ERRORE removeFileServer: file non presente nel server", EPERM, UNLOCK(&globalMutex))

    IS_FILE_LOCKED(UNLOCK(&globalMutex))


    CSA(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "removeFileServer: client non ha aperto il file",
        EPERM, UNLOCK(&globalMutex))

    //elimino il singolo file
    CSA(icl_hash_delete(hashPtrF, (void *) pathname, free, freeFileData) == -1, "removeFileServer: icl_hash_delete", EPERM, END_WRITE_LOCK)

    UNLOCK(&globalMutex)

    return 0;
}

//Rimuove il Fd e la lock del client quando questo chiude la connessione
int removeClientInfo(icl_hash_t *hashPtrF, int clientFd)
{
    LOCK(&globalMutex)
    CSA(hashPtrF == NULL, "ERRORE removeClientInfo: hashPtrF == NULL", EINVAL, UNLOCK(&globalMutex))

    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    icl_hash_foreach(hashPtrF, k, entry, key, newFile)
    {
            //--elimino la lock--
            if(newFile->lockFd == clientFd)
            {
                setUnlockFile(newFile);
            }
            else
            {
                //elimino clientId dalla coda se è presente
                deleteDataQueue(&(newFile->fdLock_TestaPtr), &(newFile->fdLock_CodaPtr), clientFd);
            }

            //elimino l'fd dalla lista dei fd che hanno fatto la open
            deleteSortedList(&(newFile->fdOpen_SLPtr), clientFd);
    }

    UNLOCK(&globalMutex);
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

    free(serverFileF);
}

void stampaHash(icl_hash_t *hashPtr) {
    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    printf("---------------------stampaHash---------------------\n");
    icl_hash_foreach(hashPtr, k, entry, key, newFile) {
            printf("---------------------%s---------------------\n", key);
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

            puts("");
            RwLock_t tempFileLock = newFile->fileLock;
            printf("num_readers_active: %d\n", tempFileLock.num_readers_active);
            printf("num_writers_waiting: %d\n", tempFileLock.num_writers_waiting);
            printf("num_readers_active: %d\n", tempFileLock.writer_active);


            printf("---------------------------------------------\n");
        }
    printf("-------------------finestampaHash-------------------\n");
}
