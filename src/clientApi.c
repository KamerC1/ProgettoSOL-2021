#include "../include/clientAPI.h"
#include <stdbool.h>

bool EN_STDOUT = true; //indica se stampare se abilitare -p
#define STAMPA_STDOUT(val) if(EN_STDOUT == true) {val;}
#define ES_NEG if(EN_STDOUT == true) puts("Esito negativo");
#define ES_POS if(EN_STDOUT == true) puts("Esito positivo");

//se la connessione viene rifiutata ritorna -1.
int openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    STAMPA_STDOUT(puts("\nOperazione: openConnection"))
    if(sockname == NULL || strlen(sockname) >= MAX_BYTES_SOCKNAME || msec < 0)
    {
        ES_NEG
        PRINT("openConnection: Argomenti invalidi")
        errno = EINVAL;
        return -1;
    }
    STAMPA_STDOUT(printf("Sockname: %s\n", sockname))

    //Creazione socket - termina tutto il processo
    FD_SOCK = socket(AF_UNIX, SOCK_STREAM, 0);
    CSA(FD_SOCK == -1, "openConnection: socket()", errno, ES_NEG)

    //Connect
    struct sockaddr_un sckAddr;
    strncpy(sckAddr.sun_path, sockname, 108);
    sckAddr.sun_family = AF_UNIX;


    while(connect(FD_SOCK, (struct sockaddr *) &sckAddr, sizeof(sckAddr)) == -1)
    {
        CSA(errno != ENOENT, "openConnection: connect()", errno, ES_NEG)
        if(errno == ENOENT)
        {
            printf("Impossibile connettersi al server. Riprovo tra %d millisecondi\n", msec);
        }

        time_t nowTime = time(NULL);
        CSA(nowTime > abstime.tv_sec, "openConnection: tempo assoluto oltrepassato", EAGAIN, ES_NEG)

        usleep(1000 * msec); //faccio passare msec millisecondi
    }
    strncpy(SOCKNAME, sockname, MAX_BYTES_SOCKNAME);

    int esitoConnessione;
    READN(FD_SOCK, &esitoConnessione, sizeof(int), "Errore: readn()")

    if(esitoConnessione == CONNESSIONE_RIFIUTATA)
    {
        puts("Connessione rifiutata");
        errno = EACCES;
        ES_NEG
        return -1;
    }

    ES_POS;

    return 0;
}

int closeConnection(const char *sockname)
{
    STAMPA_STDOUT(puts("\nOperazione: closeConnection"))
    if(sockname == NULL || strncmp(sockname, SOCKNAME, MAX_BYTES_SOCKNAME))
    {
        ES_NEG
        PRINT("closeConnection: Argomento invalidi")
        errno = EINVAL;
        return -1;
    }
    STAMPA_STDOUT(printf("Sockname: %s\n", sockname))

    CSA(close(FD_SOCK), "closeConnection: close()", errno, ES_NEG)
    ES_POS

    return 0;
}

//ritorna -1 anche se fallisce il server
int openFile(const char* pathname, int flags)
{
    STAMPA_STDOUT(puts("\nOperazione: openFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "openFile: checkPathname", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("File: %s\n", pathname))
    CSA(flags != O_OPEN && flags != O_CREATE && flags != O_LOCK && flags != O_CREATE + O_LOCK && flags != O_CREATE + O_OPEN, "openFile: flag sbagliata", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("Flags: %d\n", flags))

    //invia l'operazione della API
    int operazione = API_OPENFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "openFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "openFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "openFile: writen()")

    //Invia la flag
    WRITEN(FD_SOCK, &flags, sizeof(int), "openFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "Errore: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore esito openFile", esitoAPI, ES_POS)
    ES_POS

    return 0;
}

//ritorna -1 anche se fallisce il server
int readFile(const char *pathname, char **buf, size_t *size)
{
    STAMPA_STDOUT(puts("\nOperazione: readFile"))


    //in caso di errore, buf = NULL, size = 0
    *size = 0;
    *buf = NULL;
    //controllo argomento
    CSA(checkPathname(pathname) == -1, "readFile: checkPathname", EINVAL, ES_NEG)

    //invia l'operazione della API
    int operazione = API_READFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "readFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "readFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "readFile: writen()")

    STAMPA_STDOUT(printf("File: %s", pathname))


    //ricezione esito dell'operazione per l'esecuzione di readFileServer
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "readFile: readn(FD_SOCK, &esitoAPI, sizeof(int))")
    CSA(esitoAPI != API_SUCCESS, "Errore readFileServer", esitoAPI, ES_NEG)


    //memorizzo i dati in *buf e *size
    READN(FD_SOCK, size, sizeof(size_t), "readFile: readn(FD_SOCK, size, sizeof(size_t))")
    if(*size > 0)
    {
        *buf = malloc(sizeof(char) * (*size));
        CSA(*buf == NULL, "readFile: malloc", errno, ES_NEG)
        READN(FD_SOCK, *buf, *size, "readFile: READN(FD_SOCK, *buf, *size)")
    }

    //ricezione esito per l'invio del buffer
    READN(FD_SOCK, &esitoAPI, sizeof(int), "readFile: readn(FD_SOCK, &esitoAPI, sizeof(int))")
    CSA(esitoAPI != API_SUCCESS, "Errore readFileServer", esitoAPI, if(*buf != NULL) free(*buf); ES_NEG)
    PRINT("readFile eseguita con sueccesso")

    //stampa bytes letti
    long returnValueRead;
    READN(FD_SOCK, &returnValueRead, sizeof(long), "writeFile: readn()")
    STAMPA_STDOUT(printf("Bytes letti: %ld\n", returnValueRead))

    ES_POS

    return 0;
}

int copyFileToDir(const char *pathname, const char *dirname)
{
    STAMPA_STDOUT(puts("\nOperazione: copyFileToDir"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "writeFile: checkPathname", EINVAL, ES_NEG)
    CSA(dirname == NULL, "writeFile: checkPathname", EINVAL, ES_NEG)

    STAMPA_STDOUT(printf("File: %s\n", pathname))
    STAMPA_STDOUT(printf("Directory: %s\n", dirname))


    //invia l'operazione della API
    int operazione = API_COPY_FILE_TODIR;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "writeFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "writeFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "writeFile: writen()")

    int dirnameBytes = strlen(dirname) + 1; //+1 per '\0'
    char dirnamTemp[dirnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(dirnamTemp, dirname, dirnameBytes);
    WRITEN(FD_SOCK, &dirnameBytes, sizeof(int), "writeFile: writen()")
    WRITEN(FD_SOCK, dirnamTemp, dirnameBytes, "writeFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "writeFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore writeFile", esitoAPI, ES_NEG)
    PRINT("writeFile eseguita con sueccesso")

    //stampa bytes letti
    long returnValueCopy;
    READN(FD_SOCK, &returnValueCopy, sizeof(long), "writeFile: readn()")
    STAMPA_STDOUT(printf("Bytes letti: %ld\n", returnValueCopy))
    ES_POS

    return 0;
}

//ritorna -1 anche se fallisce il server
int readNFiles(int N, const char *dirname)
{
    STAMPA_STDOUT(puts("\nOperazione: readNFiles"))
    STAMPA_STDOUT(printf("N: %d\n", N))

    //controllo argomento
    CSA(dirname == NULL, "readNFiles: dirname == NULL", EINVAL, ES_NEG)

    STAMPA_STDOUT(printf("Directory: %s\n", dirname))

    //invia l'operazione della API
    int operazione = API_READNFILES;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "readNFiles: writen()")

    //Invia "N"
    WRITEN(FD_SOCK, &N, sizeof(int), "readNFiles: writen()")

    //Invia dirname
    int dirnameBytes = strlen(dirname) + 1; //+1 per '\0'
    char temp[dirnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, dirname, dirnameBytes);
    WRITEN(FD_SOCK, &dirnameBytes, sizeof(int), "readNFiles: writen(FD_SOCK, &pathnameBytes, sizeof(int))")
    WRITEN(FD_SOCK, temp, dirnameBytes, "readNFiles: writen(FD_SOCK, temp, pathnameBytes)")

    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "readNFiles: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore readNFiles", esitoAPI, ES_NEG)
    PRINT("readNFiles eseguita con sueccesso")

    //stampa numero di bytes letti
    unsigned long long int bytesLetti;
    READN(FD_SOCK, &bytesLetti, sizeof(unsigned long long int), "writeFile: readn()")
    STAMPA_STDOUT(printf("Bytes letti: %lld\n", bytesLetti))

    //stampa numero file letti
    int returnValueRead;
    READN(FD_SOCK, &returnValueRead, sizeof(int), "writeFile: readn()")
    STAMPA_STDOUT(printf("file letti: %d\n", returnValueRead))

    ES_POS

    return 0;
}

//ritorna -1 anche se fallisce il server
int writeFile(const char *pathname, const char *dirname)
{
    STAMPA_STDOUT(puts("\nOperazione: writeFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "writeFile: checkPathname", EINVAL, ES_NEG)


    //invia l'operazione della API
    int operazione = API_WRITEFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "writeFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "writeFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "writeFile: writen()")

    //Invia dirname
    if (dirname == NULL)
    {
        int argumentNull = ARGUMENT_NULL;
        WRITEN(FD_SOCK, &argumentNull, sizeof(int), "writeFile: writen()")
    }
    else
    {
        int dirnameBytes = strlen(dirname) + 1; //+1 per '\0'
        char dirnamTemp[dirnameBytes]; //writen non può prendere una costante [eliminare]
        strncpy(dirnamTemp, dirname, dirnameBytes);
        WRITEN(FD_SOCK, &dirnameBytes, sizeof(int), "writeFile: writen()")
        WRITEN(FD_SOCK, dirnamTemp, dirnameBytes, "writeFile: writen()")
    }


    //stampa -p
    STAMPA_STDOUT(printf("File: %s\n", pathname))
    if(dirname != NULL)
        STAMPA_STDOUT(printf("Dirname: %s\n", dirname))

    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "writeFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore writeFile", esitoAPI, ES_NEG)
    PRINT("writeFile eseguita con sueccesso")

    //stampa bytes
    long returnValueWrite;
    READN(FD_SOCK, &returnValueWrite, sizeof(long), "writeFile: readn()")
    STAMPA_STDOUT(printf("Bytes scritti: %ld\n", returnValueWrite))

    ES_POS
    return 0;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname)
{
    STAMPA_STDOUT(puts("\nOperazione: appendToFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "appendToFile: checkPathname", EINVAL, ES_NEG)
    CSA(buf == NULL || strlen(buf) + 1 != size, "appendToFile: buf == NULL || strlen(buf) + 1", EINVAL, ES_NEG) //"size": numero byte di buf e non il numero di caratteri

    //invia l'operazione della API
    int operazione = API_APPENDTOFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "appendToFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "appendToFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "appendToFile: writen()")

    //Invia size e buf
    WRITEN(FD_SOCK, &size, sizeof(size_t), "appendToFile: writen()")
    WRITEN(FD_SOCK, buf, size, "appendToFile: writen()")


    //Invia dirname
    if (dirname == NULL)
    {
        int argumentNull = ARGUMENT_NULL;
        WRITEN(FD_SOCK, &argumentNull, sizeof(int), "appendToFile: writen()")
    }
    else
    {
        int dirnameBytes = strlen(dirname) + 1; //+1 per '\0'
        char dirnamTemp[dirnameBytes]; //writen non può prendere una costante [eliminare]
        strncpy(dirnamTemp, dirname, dirnameBytes);
        WRITEN(FD_SOCK, &dirnameBytes, sizeof(int), "appendToFile: writen()")
        WRITEN(FD_SOCK, dirnamTemp, dirnameBytes, "appendToFile: writen()")
    }

    //stampa -p
    STAMPA_STDOUT(printf("File: %s\n", pathname))
    STAMPA_STDOUT(printf("buffer: %s\n", (char *) buf))
    STAMPA_STDOUT(printf("Sieze buffer: %ld\n", size))
    if(dirname != NULL)
        STAMPA_STDOUT(printf("Dirname: %s\n", dirname))



    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "appendToFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore appendToFile", esitoAPI, ES_NEG)

    ES_POS

    return 0;
}

int closeFile(const char *pathname)
{
    STAMPA_STDOUT(puts("\nOperazione: closeFile"))


    //controllo argomento
    CSA(checkPathname(pathname) == -1, "closeFile: checkPathname", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("File: %s\n", pathname))


    //invia l'operazione della API
    int operazione = API_CLOSEFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "closeFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "closeFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "closeFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "Errore: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore esito closeFile", esitoAPI, ES_NEG)
    ES_POS

    return 0;
}

int lockFile(const char *pathname)
{
    STAMPA_STDOUT(puts("\nOperazione: lockFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "lockFile: checkPathname", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("File: %s\n", pathname))


    //invia l'operazione della API
    int operazione = API_LOCKFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "lockFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "lockFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "lockFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "lockFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore esito lockFile", esitoAPI, ES_NEG)
    ES_POS
    return 0;
}

int unlockFile(const char *pathname)
{
    STAMPA_STDOUT(puts("\nOperazione: unlockFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "unlockFile: checkPathname", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("File: %s\n", pathname))


    //invia l'operazione della API
    int operazione = API_UNLOCKFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "unlockFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "unlockFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "unlockFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "unlockFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "Errore esito unlockFile", esitoAPI, ES_NEG)
    ES_POS
    return 0;
}

int removeFile(const char *pathname)
{
    STAMPA_STDOUT(puts("\nOperazione: removeFile"))

    //controllo argomento
    CSA(checkPathname(pathname) == -1, "removeFile: checkPathname", EINVAL, ES_NEG)
    STAMPA_STDOUT(printf("File: %s\n", pathname))


    //invia l'operazione della API
    int operazione = API_REMOVEFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "removeFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "removeFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "removeFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "removeFile: readn()")
    CSA(esitoAPI != API_SUCCESS, "removeFile esito closeFile", esitoAPI, ES_NEG)
    ES_POS
    return 0;
}

//restituisce: -1 se il file "pathname" non è presente in hashPtrF
//0 se clientfd NON ha aperto il file "pathname"
//1 se clientfd ha aperto il file "pathname"
int isPathPresent(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "removeFile: checkPathname", EINVAL)


    //invia l'operazione della API
    int operazione = API_ISFILE_PRESENT;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "removeFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "removeFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "removeFile: writen()")


    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "removeFile: readn()")

    return esitoAPI;
}


size_t getSizeFileByte(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "getSizeFileByte: checkPathname", EINVAL)


    //invia l'operazione della API
    int operazione = API_GET_FILE_SIZEBYTE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "getSizeFileByte: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "getSizeFileByte: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "getSizeFileByte: writen()")


    //ricezione esito dell'operazione del server
    size_t esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(size_t), "getSizeFileByte: readn()")

    return esitoAPI;
}

//Client manda una richiesta al server per eliminare tutte le sue informazioni memorizzate(fd e lock)
int removeClientInfoAPI()
{
    //invia l'operazione della API
    int operazione = API_REMOVE_CLIENT_INFO;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "removeClientInfoAPI: writen()")

    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "removeClientInfoAPI: readn()")
    CS(esitoAPI != API_SUCCESS, "removeClientInfoAPI esito closeFile", esitoAPI)
    PRINT("removeClientInfoAPI eseguita con sueccesso")

    return 0;
}
