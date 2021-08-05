#include <stdio.h>
#include <stdbool.h>
#include "hash/icl_hash.c"
#include "sortedList/sortedList.c"
#include "uitls/util.h"

//numero di file che il server può contenere attraverso la tabella hash (dato da prendere da config.txt) [eliminare]
#define NUMFILE 10
#define O_CREATE 1
#define O_LOCK 10
#define O_OPEN 100 //da usare se si vuole aprire il file senza crearlo o usare la lock

#define STAMPA_ERRORE 1

struct file {
    char *path;
    char *fileContent;
    size_t sizeFileByte; //conta anche il '\0'
    bool canWriteFile; //1: se si può fare la WriteFile(), 0 altrimenti

    //lista che memorizza l'fd dei client che hanno chiamato la open
    Nodo *fdOpen_SLPtr;

};
typedef struct file File;

int writeFile(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
void fillStructFile(File *serverFileF);
int openFile(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd);
File *createFile(const char *pathname, int clientFd);
int closeFile(const char *pathname, icl_hash_t *hashPtrF, int clientFd);
void stampaHash(icl_hash_t *hashPtr);
void freeFileData(void *serverFile);
int readFile(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd);

int readNFiles(int N, const char *dirname, icl_hash_t *hashPtrF);
void createReadNFiles(int N, icl_hash_t *hashPtrF);

int appendToFile(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd);
int closeFile(const char *pathname, icl_hash_t *hashPtrF, int clientFd);



int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("Inserire il pathname");
        exit(EXIT_FAILURE);
    }

    icl_hash_t *hashPtr = icl_hash_create(NUMFILE, NULL, NULL);


    SYSCALL(openFile(argv[1], O_CREATE, hashPtr, 1), "ERRORE: ")
    SYSCALL(openFile(argv[1], O_OPEN, hashPtr, 2), "ERRORE: ")

    SYSCALL(writeFile(argv[1], NULL, hashPtr, 1), "ERRORE: ")


    stampaHash(hashPtr);

    icl_hash_destroy(hashPtr, NULL, freeFileData);

    return 0;
}

//da finire [controllare]
int openFile(const char *pathname, int flags, icl_hash_t *hashPtrF, int clientFd) {
    //argomenti invalidi
    COND_SETERR(!strlen(pathname) || hashPtrF == NULL || clientFd < 0,
                "ERRORE: !strlen(pathname) || hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = NULL;
    switch (flags) {
        case O_OPEN:
            serverFile = icl_hash_find(hashPtrF, (void *) pathname);
            //il file non è presente nella hash table
            COND_SETERR(serverFile == NULL, "O_OPEN: File non creato", EPERM)
            //clientFd ha già aperto la lista
            COND_SETERR(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == 0, "O_OPEN: file già aperto dal client",
                        EPERM)


            COND_SETERR(insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1, "O_OPEN: File non creato", EPERM)

            break;

        case O_CREATE:
            //il file è già presente nella hash table
            COND_SETERR(icl_hash_find(hashPtrF, (void *) pathname) != NULL, "O_CREATE: File non creato", EPERM)


            serverFile = createFile(pathname, clientFd);
            COND_SETERR(icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile) == NULL,
                        "Errore: icl_hash_insert", EPERM)

            break;

        case O_LOCK:
            COND_SETERR(icl_hash_find(hashPtrF, (void *) pathname) == NULL,
                        "O_LOCK: File non presente nella tabella hash", EPERM)


            //GESTIONE LOCK [controllare]
            break;

        case (O_CREATE + O_LOCK):
            //il file è già presente nella hash table
            COND_SETERR(icl_hash_find(hashPtrF, (void *) pathname) != NULL, "O_CREATE+O_LOCK: File non creato", EPERM)


            serverFile = createFile(pathname, clientFd);
            icl_hash_insert(hashPtrF, (void *) pathname, (void *) serverFile);
            //GESTIONE LOCK [controllare]
            break;

        default:
            //la flag non corrisponde
            PRINT("flag sbagliata");

            errno = EINVAL;
            return -1;
            break;
    }

    serverFile->canWriteFile = 1;

    return 0;
}

int readFile(const char *pathname, char **buf, size_t *size, icl_hash_t *hashPtrF, int clientFd)
{
    //in caso di errore, buf = NULL, size = 0
    *size = 0;

    //Argomenti invalidi
    //non c'è bisogno di controllare: !strlen(pathname) perché lo si fa dopo con icl_hash_find [eliminare]
    COND_SETERR(pathname == NULL || *buf != NULL || hashPtrF == NULL || clientFd < 0,
                "ERRORE: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL)

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    COND_SETERR(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
                "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM);



    *size = serverFile->sizeFileByte;
    COND_SETERR((*buf = malloc(*size)) == NULL, "readFile: *buf = malloc(*size)", EPERM)
    memset(*buf, '\0', *size);

    if(strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte) == NULL)
    {
        //in caso di errore, libero la memoria
        puts("strncpy(*buf, serverFile->fileContent, serverFile->sizeFileByte) = NULL");
        free(*buf);
        errno = EPERM;
        return -1;
    }

    //readFile terminata con successo ==> non posso più fare la writeFile
    serverFile->canWriteFile = 0;

    return 0;
}

int readNFiles(int N, const char *dirname, icl_hash_t *hashPtrF)
{
    COND_SETERR(hashPtrF == NULL,"ERRORE: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL);
    COND_SETERR(dirname == NULL, "readNFiles: dirname == NULL", EINVAL)

    //imposta il numero di file da leggere
    if(N <= 0 || hashPtrF->nentries < N)
        N = hashPtrF->nentries;

    //==Calcola il path dove lavora il processo==
    //calcolo la lunghezza del path [eliminare]
    char *temp = NULL;
    int pathLength = strlen(temp = getcwd(NULL, MAX_PATH_LENGTH)); //il +1 è già compreso in MAX_PATH_LENGTH
    free(temp);
    //cambio path [eliminare]
    char processPath[pathLength+1]; //+1 per '\0'
    getcwd(processPath, pathLength+1); //+1 per '\0'

    DIR * directory;
    RETURN_NULL_SYSCALL(directory, opendir(dirname), "Errore: opendir")
    chdir(dirname);

    createReadNFiles(N, hashPtrF);

    chdir(processPath); //ritorno alla directory originaria
    closedir(directory);

    return 0;
}

//funzione di supporto a readNFiles. Crea i file letti dal server e ne copia il contenuto
void createReadNFiles(int N, icl_hash_t *hashPtrF)
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
            return;
        }
        NfileCreated++;

        FILE *diskFile;
        //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
        RETURN_NULL_SYSCALL(diskFile, fopen(basename(serverFile->path), "w"), "Errore: fopen(basename(serverFile->path)")
        fputs(serverFile->fileContent, diskFile);

        SYSCALL_NOTZERO(fclose(diskFile), "fclose(diskFile)")
    }
}

//alloca e inizializzata una struttura File
File *createFile(const char *pathname, int clientFd) {
    //non usare COND_SETERR perché non ritora UN INTERO [eliminare]
    if (pathname == NULL || !strlen(pathname) || clientFd < 0) {
        PRINT("ERRORE: !strlen(pathname) clientFd < 0");

        errno = EINVAL;
        return NULL;
    }

    File *serverFile;
    RETURN_NULL_SYSCALL(serverFile, malloc(sizeof(File)), "Errore: newFile = malloc(sizeof(struct file))")

    //--inserimento pathname--
    int strlenPath = strlen(pathname);
    RETURN_NULL_SYSCALL(serverFile->path, malloc(sizeof(char) * strlenPath + 1),
                        "Errore: newFile = malloc(sizeof(struct file))")
    strncpy(serverFile->path, pathname, strlenPath + 1);

    serverFile->fileContent = NULL;
    serverFile->sizeFileByte = 0;
    serverFile->canWriteFile = 0;

    serverFile->fdOpen_SLPtr = NULL;
    insertSortedList(&(serverFile->fdOpen_SLPtr), clientFd);

    return serverFile;
}

//manca buona parte della funzione - [controllare]
int writeFile(const char *pathname, const char *dirname, icl_hash_t *hashPtrF, int clientFd) {
    //Argomenti invalidi
    //non c'è bisogno di controllare: !strlen(pathname) perché lo si fa dopo con icl_hash_find [eliminare]
    COND_SETERR(pathname == NULL || hashPtrF == NULL || clientFd < 0,
                "ERRORE: pathname == NULL || hashPtrF == NULL || clientFd < 0", EINVAL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    COND_SETERR(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
                "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM)
    //l'ultima operazione non era una open || il file non è vuoto
    COND_SETERR(serverFile->canWriteFile == 0 || serverFile->sizeFileByte != 0,
                "l'ultima operazione non era una open || il file non è vuoto", EPERM)


    serverFile->canWriteFile = 0;
    fillStructFile(serverFile);

    return 0;
}

//riempe i campi dello struct "File" a partire da file in disk con pathname: "pathname". Restituisce il dato "File"
void fillStructFile(File *serverFileF) {
    //===DA CAMBIARE e fare che fillStructFile() ritorni un int === [controllare]
    if (serverFileF == NULL) {
        PRINT("serverFileF == NULL");
        exit(EXIT_FAILURE);
    }

    FILE *diskFile;
    RETURN_NULL_SYSCALL(diskFile, fopen(serverFileF->path, "r"), "Errore: diskFile = fopen(pathname, \"r\")")

    //-Inserimento sizeFileByte-
    rewind(diskFile); //forse conviene in caso non non parta dall'inizio il puntatore - [eliminare]
    SYSCALL(fseek(diskFile, 0L, SEEK_END), "fseek(diskFile, 0L, SEEK_END);");
    RETURN_SYSCALL(serverFileF->sizeFileByte, ftell(diskFile), "DiskFileLength = ftell(diskFile)")
    rewind(diskFile); //riposiziono il puntatore - [eliminare]

    size_t numCharFile = serverFileF->sizeFileByte; //numero di caratteri presente nel file
    serverFileF->sizeFileByte += 1; //memorizza anche il '\0' (perché anche esso occupa spazio)

    //-Inserimento fileContent-
    RETURN_NULL_SYSCALL(serverFileF->fileContent, malloc(sizeof(char) * numCharFile + 1),
                        "Errore: newFile = malloc(sizeof(struct file))")
    memset(serverFileF->fileContent, '\0', numCharFile + 1);
    size_t readLength = fread(serverFileF->fileContent, 1, numCharFile,
                              diskFile); //size: 1 perché si legge 1 byte alla volta [eliminare]
    if (readLength != numCharFile || ferror(diskFile))
    {
        puts("Errore: fread(serverFile->fileContent, 1, serverFile->sizeFileByte, diskFile)");
        exit(EXIT_FAILURE);
    }

    SYSCALL_NOTZERO(fclose(diskFile), "fclose(diskFile)")
}

int closeFile(const char *pathname, icl_hash_t *hashPtrF, int clientFd) {
    //la correttezza di pathname viene controllata dopo [eliminare]
    COND_SETERR(pathname == NULL || hashPtrF == NULL || clientFd < 0,
                "ERRORE close: pathname == NULL || hashPtrF == NULL || clientFd < 0", EPERM)
    //file non presente nel server
    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    COND_SETERR(serverFile == NULL, "ERRORE close: file non presente nel server", EPERM)
    //il client non ha aperto il file
    COND_SETERR(findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1, "CLOSE: client non ha aperto il file", EPERM)


    //Client "chiude" il file aperto
    COND_SETERR(deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd) == -1,
                "CLOSE: deleteSortedList(&(serverFile->fdOpen_SLPtr), clientFd)", EPERM)
    serverFile->canWriteFile = 0;

    return 0;
}

int appendToFile(const char *pathname, char *buf, size_t size, const char *dirname, icl_hash_t *hashPtrF, int clientFd) {
    //Argomenti invalidi
    //"size": numero di byte di buf e non il numero di caratteri
    COND_SETERR(pathname == NULL || buf == NULL || strlen(buf) + 1 != size,
                "appendToFile: pathname == NULL || buf == NULL || strlen(buf)+1 != size", EINVAL);
    COND_SETERR(hashPtrF == NULL || clientFd < 0, "appendToFile: hashPtrF == NULL || clientFd < 0", EINVAL);

    File *serverFile = icl_hash_find(hashPtrF, (void *) pathname);
    //il file non è presente nella hash table o non è stato aperto da clientFd
    COND_SETERR(serverFile == NULL || findSortedList(serverFile->fdOpen_SLPtr, clientFd) == -1,
                "il file non è presente nella hash table o non è stato aperto da clientFd", EPERM)


    REALLOC(serverFile->fileContent, sizeof(char) * (serverFile->sizeFileByte + size -
                                                     1)) //il +1 è già contato in "serverFile->sizeFileByte" e in size (per questo motivo vien fatto -1)
    strncat(serverFile->fileContent, buf,
            size - 1); //sovvrascrive in automatico il '\0' di serverFile->fileContent e ne aggiunge uno alla fine
    //Aggiorno serverFile->sizeFileByte
    serverFile->sizeFileByte = serverFile->sizeFileByte + size - 1;

    serverFile->canWriteFile = 0;

    return 0;
}

//elimina tutti i dati compresi in serverFile (ed elimina lo stesso serverFile)
void freeFileData(void *serverFile)
{
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
    icl_hash_foreach(hashPtr, k, entry, key, newFile) {
        printf("-----------------------------------------------\n");
        printf("Path: %s\n", newFile->path);
        printf("\nfileContent:\n%s\n\n", newFile->fileContent);
        printf("sizeFileByte: %ld\n", newFile->sizeFileByte);
        stampaSortedList(newFile->fdOpen_SLPtr);
        printf("-----------------------------------------------\n");
    }
}
