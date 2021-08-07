#include <stdio.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/unistd.h>

#include "../include/serverAPI.h"
#include "../utils/util.h"


//da finire [controllare]
int openFileServer(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd)
{
    //argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)
    CS(hashPtrF == NULL || clientFd < 0,"openFile: hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = NULL;
    switch (flags)
    {
        case O_OPEN:
            serverFile = icl_hash_find(hashPtrF, (void *) pathname);
            //il file non è presente nella hash table
            CS(serverFile == NULL, "O_OPEN: File non presente", EPERM)
            //clientFd ha già aperto la lista
            CS(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_OPEN: file già aperto dal client",
                        EPERM)


            insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);

            break;

        case O_CREATE:
            //il file è già presente nella hash table
            CS(icl_hash_find(hashPtrF, (void *) pathname) != NULL, "O_CREATE: File già presente", EPERM)


            serverFile = createFile(pathname, clientFd);
            CS(serverFile == NULL, "O_CREATE: impossibile creare la entry nella hash", EPERM)
            CS(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile) == NULL,
                        "O_CREATE: impossibile inserire File nella hash", EPERM)

            break;

        case O_LOCK:
            CS(icl_hash_find(hashPtrF, (void *) pathname) == NULL,
                        "O_LOCK: File non presente nella tabella hash", EPERM)


            //GESTIONE LOCK [controllare]
            break;

        case (O_CREATE + O_LOCK):
            //il file è già presente nella hash table
            CS(icl_hash_find(hashPtrF, (void *) pathname) != NULL, "O_CREATE+O_LOCK: File non creato", EPERM)


            serverFile = createFile(pathname, clientFd);
            CS(serverFile == NULL, "O_CREATE + O_LOCK: impossibile creare la entry nella hash", EPERM)
            CS(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile) == NULL,
                        "O_CREATE + O_LOCK: impossibile inserire File nella hash", EPERM)
            //GESTIONE LOCK [controllare]
            break;

        default:
            //la flag non corrisponde
            PRINT("flag sbagliata")

            errno = EINVAL;
            return -1;
            break;
    }

    serverFile->canWriteFile = 1;

    return 0;
}
//Funzione di supporto a openFIle: alloca e inizializzata una struttura File
static File *createFile(const char *pathname, int clientFd)
{
    CSN(checkPathname(pathname), "createFile: pathname sbaglito", EINVAL)
    CSN(clientFd < 0, "clientFd < 0", EINVAL)



    File *serverFile;
    RETURN_NULL_SYSCALL(serverFile, malloc(sizeof(File)), "createFile: serverFile = malloc(sizeof(File))")

    //--inserimento pathname--
    int strlenPath = strlen(pathname);
    RETURN_NULL_SYSCALL(serverFile->path, malloc(sizeof(char) * strlenPath + 1), "createFile: malloc(sizeof(char) * strlenPath + 1)")
    strncpy(serverFile->path, pathname, strlenPath + 1);

    serverFile->fileContent = NULL;
    serverFile->sizeFileByte = 0;
    serverFile->canWriteFile = 0; //viene impostato a 1 alla fine della openFile

    serverFile->fdOpen_SLPtr = NULL;
    insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);

    return serverFile;
}


int readFileServer(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd)
{
    //in caso di errore, buf = NULL, size = 0
    *size = 0;
    *buf = NULL;
    //argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)
    CS(hashPtrF == NULL || clientFd < 0,"openFile: hashPtrF == NULL || clientFd < 0", EINVAL)



    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CS(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
                "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM)



    *size = serverFile->sizeFileByte;
    RETURN_NULL_SYSCALL(*buf, malloc(*size), "readFile: *buf = malloc(*size)")
    memset(*buf, '\0', *size);

    strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte);

    //readFile terminata con successo ==> non posso più fare la writeFile
    serverFile->canWriteFile = 0;

    return 0;
}

int readNFilesServer(int N, const char *dirname, icl_hash_t *hashPtrF)
{
    CS(hashPtrF == NULL,"ERRORE: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL)
    CS(dirname == NULL, "readNFiles: dirname == NULL", EINVAL)

    //imposta il numero di file da leggere
    if(N <= 0 || hashPtrF->nentries < N)
        N = hashPtrF->nentries;

    //==Calcola il path dove lavora il processo==
    //calcolo la lunghezza del path [eliminare]
    char *temp = getcwd(NULL, MAX_PATH_LENGTH);
    CS(temp == NULL, "readNFiles: getcwd", errno)
    int pathLength = strlen(temp); //il +1 è già compreso in MAX_PATH_LENGTH
    free(temp);
    char processPath[pathLength+1]; //+1 per '\0'
    CS(getcwd(processPath, pathLength+1) == NULL, "readNFiles: ", errno) //+1 per '\0'

    //cambio path [eliminare]
    DIR * directory = opendir(dirname);
    CS(directory == NULL, "directory = opendir(dirname): ", errno) //+1 per '\0'

    CSA(chdir(dirname) == -1, "readNFiles: chdir(dirname)", errno, CLOSEDIR(directory))
    CSA(createReadNFiles(N, hashPtrF) == -1, "readNFiles: createReadNFiles", errno, CLOSEDIR(directory))
    CSA(chdir(processPath) == -1, "readNFiles: chdir(processPath)", errno, CLOSEDIR(directory))


    CLOSEDIR(directory)

    //canWrite viene aggiornato in createReadNFiles

    return 0;
}

//funzione di supporto a readNFiles. Crea i file letti dal server e ne copia il contenuto
int createReadNFiles(int N, icl_hash_t *hashPtrF)
{
    int NfileCreated = 0;

    int k;
    icl_entry_t *entry;
    char *key;
    File *serverFile;
    icl_hash_foreach(hashPtrF, k, entry, key, serverFile)
    {
        if(NfileCreated >= N)
        {
            return 0;
        }
        NfileCreated++;

        //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
        FILE *diskFile = fopen(basename(serverFile->path), "w");
        CS(diskFile == NULL, "createReadNFiles: fopen()", errno)
        if(fputs(serverFile->fileContent, diskFile) == EOF)
        {
            puts("createReadNFiles: fputs");
            SYSCALL_NOTZERO(fclose(diskFile), "createReadNFiles: fclose() - termino processo")
        }

        SYSCALL_NOTZERO(fclose(diskFile), "createReadNFiles: fclose() - termino processo")

        serverFile->canWriteFile = 0;
    }

    return 0;
}


//manca buona parte della funzione - [controllare]
int writeFileServer(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd)
{
    //Argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)

    CS(hashPtrF == NULL || clientFd < 0,
                "ERRORE: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    CS(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
                "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM)
    //l'ultima operazione non era una open || il file non è vuoto
    CS(serverFile->canWriteFile == 0 || serverFile->sizeFileByte != 0,
                "l'ultima operazione non era una open || il file non è vuoto", EPERM)


            //ripristino "sizeFileByte" in caso di errore perché fillStructFile() non lo fa.
    CSA(fillStructFile(serverFile) == -1, "", errno, serverFile->sizeFileByte = 0)

    serverFile->canWriteFile = 0;

    return 0;
}

//riempe i campi dello struct "File" a partire da file in disk con pathname: "pathname". Restituisce il dato "File"
int fillStructFile(File *serverFileF)
{
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
    size_t readLength = fread(serverFileF->fileContent, 1, numCharFile,diskFile); //size: 1 perché si legge 1 byte alla volta [eliminare]
    //Controllo errore fread
    CS(readLength != numCharFile || ferror(diskFile), "fillStructFile: fread", errno)

    FCLOSE(diskFile)
    return 0;
}

int closeFileServer(const char *pathname, icl_hash_t *hashPtrF, int clientFd)
{
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)
    CS(hashPtrF == NULL || clientFd < 0, "ERRORE close: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CS(serverFile == NULL, "ERRORE close: file non presente nel server", EPERM)
    CS(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "CLOSE: client non ha aperto il file", EPERM)



    //Client "chiude" il file aperto
    CS(deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1,
                "CLOSE: deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd)", EPERM)
    serverFile->canWriteFile = 0;

    return 0;
}

int appendToFileServer(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd)
{
    //Argomenti invalidi
    CS(checkPathname(pathname), "openFile: pathname sbaglito", EINVAL)
    CS(buf == NULL || strlen(buf) + 1 != size, "appendToFile: buf == NULL || strlen(buf) + 1", EINVAL) //"size": numero byte di buf e non il numero di caratteri
    CS(hashPtrF == NULL || clientFd < 0, "appendToFile: hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    CS(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM)


    REALLOC(serverFile->fileContent, sizeof(char) * (serverFile->sizeFileByte + size - 1)) //il +1 è già contato in "serverFile->sizeFileByte" e in size (per questo motivo vien fatto -1)
    CS(strncat(serverFile->fileContent, buf, size - 1) == NULL, "appendToFile: strncat", errno) //sovvrascrive in automatico il '\0' di serverFile->fileContent e ne aggiunge uno alla fine
    serverFile->sizeFileByte = serverFile->sizeFileByte + size - 1;

    serverFile->canWriteFile = 0;

    return 0;
}


//elimina tutti i dati compresi in serverFile (ed elimina lo stesso serverFile)
void freeFileData(void *serverFile) {
    File *serverFileF = serverFile;

    if (serverFileF->path != NULL)
        free(serverFileF->path);
    if (serverFileF->fileContent != NULL)
        free(serverFileF->fileContent);
    if (serverFileF->fdOpen_SLPtr != NULL)
        freeSortedList(&(serverFileF->fdOpen_SLPtr));

    free(serverFileF);
}


void stampaHash(icl_hash_t *hashPtr) {
    //k viene inizializzato a 0 in icl_hash_foreach
    int k;
    icl_entry_t *entry;
    char *key;
    File *newFile;
    icl_hash_foreach(hashPtr, k, entry, key, newFile)
    {
        printf("-----------------------------------------------\n");
        printf("Path: %s\n", newFile->path);
        printf("FIle name: %s\n", basename(newFile->path));
        printf("\nfileContent:\n%s\n\n", newFile->fileContent);
        printf("sizeFileByte: %ld\n", newFile->sizeFileByte);
        stampaSortedList(newFile->fdOpen_SLPtr);
        printf("-----------------------------------------------\n");
    }
}

//Controlla se pathname è corretto
int checkPathname(const char *pathname)
{
    //basename può modificare pathname
    int lengthPath = strlen(pathname);
    char pathnameCopy[lengthPath+1];
    strncpy(pathnameCopy, pathname, lengthPath+1);

    if(pathname == NULL || strlen(pathname) >= MAX_PATH_LENGTH-1 || strlen(pathname) <= 0 || basename(pathnameCopy) == NULL || strlen(pathnameCopy) >= MAX_FILE_LENGTH-1)
        return -1;
    return 0;
}
