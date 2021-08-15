#include "../include/clientAPI.h"

//int main(int argc, char *argv[])
//{
//    CS(argc < 3, "inserire argomenti", EINVAL)
//
//    //sceglie quali istruzioni eseguire
//    int expression = 0;
////    printf("expression: ");
////    scanf("%d", &expression);
//
//
//    if(expression == 0)
//    {
//        struct timespec abstime;
//    abstime.tv_sec = time(NULL) + 10;
//    SYSCALL(openConnection(argv[1], 1000, abstime), "Errore");
//
//    char *pathname = getRealPath(argv[2]);
//    SYSCALL(openFile(pathname, O_CREATE), "Errore")
//
//    SYSCALL(closeConnection(SOCKNAME), "Errore")
//
//    }
//
////    struct timespec abstime;
////    abstime.tv_sec = time(NULL) + 10;
////    SYSCALL(openConnection(argv[1], 1000, abstime), "Errore");
////
////    char *pathname = getRealPath(argv[2]);
////    SYSCALL(openFile(pathname, O_CREATE), "Errore")
////
////    SYSCALL(openFile(pathname, O_LOCK), "Errore")
////    SYSCALL(writeFile(pathname, NULL), "Errore")
////    SYSCALL(readNFiles(0, "../test/readN/"), "Errore")
////    SYSCALL(appendToFile(pathname, "gatto", strlen("gatto")+1, NULL), "Errore")
////
////    char *buf;
////    size_t size;
////    SYSCALL(readFile(pathname, &buf, &size), "Errore")
////
////    printf("BUF: %s\n", buf);
////    printf("size: %ld\n", size);
//
////    free(buf);
//
//    sleep(3);
//
////    SYSCALL(closeConnection(SOCKNAME), "Errore")
//    return 0;
//}

int openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    if(sockname == NULL || strlen(sockname) >= MAX_BYTES_SOCKNAME || msec < 0)
    {
        PRINT("openConnection: Argomenti invalidi")
        errno = EINVAL;
        return -1;
    }


    //Creazione socket - termina tutto il processo
    FD_SOCK = socket(AF_UNIX, SOCK_STREAM, 0);
    CS(FD_SOCK == -1, "openConnection: socket()", errno)

    //Connect
    struct sockaddr_un sckAddr;
    strncpy(sckAddr.sun_path, sockname, 108);
    sckAddr.sun_family = AF_UNIX;


    while(connect(FD_SOCK, (struct sockaddr *) &sckAddr, sizeof(sckAddr)) == -1)
    {
        CS(errno != ENOENT, "openConnection: connect()", errno)
        if(errno == ENOENT)
        {
            printf("Impossibile connettersi al server. Riprovo tra %d millisecondi\n", msec);
        }

        time_t nowTime = time(NULL);
        CS(nowTime > abstime.tv_sec, "openConnection: tempo assoluto oltrepassato", EAGAIN)

        usleep(1000 * msec); //faccio passare msec millisecondi
    }
    strncpy(SOCKNAME, sockname, MAX_BYTES_SOCKNAME);
    return 0;
}

int closeConnection(const char *sockname)
{
    if(sockname == NULL || strncmp(sockname, SOCKNAME, MAX_BYTES_SOCKNAME))
    {
        PRINT("closeConnection: Argomento invalidi")
        errno = EINVAL;
        return -1;
    }

    CS(close(FD_SOCK), "closeConnection: close()", errno)
    printf("Conessione chiusa: %s\n", sockname);

    return 0;
}

//ritorna -1 anche se fallisce il server
int openFile(const char* pathname, int flags)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "openFile: checkPathname", EINVAL)
    CS(flags != O_OPEN && flags != O_CREATE && flags != O_LOCK && flags != O_CREATE + O_LOCK, "openFile: flag sbagliata", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "Errore esito openFile", esitoAPI)
    PRINT("openFile eseguita con sueccesso")

    return 0;
}

//ritorna -1 anche se fallisce il server
int readFile(const char *pathname, char **buf, size_t *size)
{
    //in caso di errore, buf = NULL, size = 0
    *size = 0;
    *buf = NULL;
    //controllo argomento
    CS(checkPathname(pathname) == -1, "readFile: checkPathname", EINVAL)

    //invia l'operazione della API
    int operazione = API_READFILE;
    WRITEN(FD_SOCK, &operazione, sizeof(int), "readFile: writen()")

    //Invia pathname
    int pathnameBytes = strlen(pathname) + 1; //+1 per '\0'
    char temp[pathnameBytes]; //writen non può prendere una costante [eliminare]
    strncpy(temp, pathname, pathnameBytes);
    WRITEN(FD_SOCK, &pathnameBytes, sizeof(int), "readFile: writen()")
    WRITEN(FD_SOCK, temp, pathnameBytes, "readFile: writen()")


    //ricezione esito dell'operazione per l'esecuzione di readFileServer
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "readFile: readn(FD_SOCK, &esitoAPI, sizeof(int))")
    CS(esitoAPI != API_SUCCESS, "Errore readFileServer", esitoAPI)

    //memorizzo i dati in *buf e *size
    READN(FD_SOCK, size, sizeof(size_t), "readFile: readn(FD_SOCK, size, sizeof(size_t))")
    if(*size > 0)
    {
        *buf = malloc(sizeof(char) * (*size));
        CS(*buf == NULL, "readFile: malloc", errno)
        READN(FD_SOCK, *buf, *size, "readFile: READN(FD_SOCK, *buf, *size)")
    }

    //ricezione esito per l'invio del buffer
    READN(FD_SOCK, &esitoAPI, sizeof(int), "readFile: readn(FD_SOCK, &esitoAPI, sizeof(int))")
    CSA(esitoAPI != API_SUCCESS, "Errore readFileServer", esitoAPI, if(*buf != NULL) free(*buf))

    PRINT("readFile eseguita con sueccesso")

    return 0;
}

int copyFileToDir(const char *pathname, const char *dirname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "writeFile: checkPathname", EINVAL)
    CS(dirname == NULL, "writeFile: checkPathname", EINVAL)

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
    CS(esitoAPI != API_SUCCESS, "Errore writeFile", esitoAPI)
    PRINT("writeFile eseguita con sueccesso")

    return 0;
}


//ritorna -1 anche se fallisce il server
int readNFiles(int N, const char *dirname)
{
    //controllo argomento
    CS(dirname == NULL, "readNFiles: dirname == NULL", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "Errore readNFiles", esitoAPI)
    PRINT("readNFiles eseguita con sueccesso")

    return 0;
}

//ritorna -1 anche se fallisce il server
int writeFile(const char *pathname, const char *dirname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "writeFile: checkPathname", EINVAL)


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



    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "writeFile: readn()")
    CS(esitoAPI != API_SUCCESS, "Errore writeFile", esitoAPI)
    PRINT("writeFile eseguita con sueccesso")

    return 0;
}

int appendToFile(const char *pathname, void *buf, size_t size, const char *dirname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "appendToFile: checkPathname", EINVAL)
    CS(buf == NULL || strlen(buf) + 1 != size, "appendToFile: buf == NULL || strlen(buf) + 1", EINVAL) //"size": numero byte di buf e non il numero di caratteri

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



    //ricezione esito dell'operazione del server
    int esitoAPI;
    READN(FD_SOCK, &esitoAPI, sizeof(int), "appendToFile: readn()")
    CS(esitoAPI != API_SUCCESS, "Errore appendToFile", esitoAPI)
    PRINT("appendToFile eseguita con sueccesso")

    return 0;
}

int closeFile(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "closeFile: checkPathname", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "Errore esito closeFile", esitoAPI)
    PRINT("closeFile eseguita con sueccesso")

    return 0;
}

int lockFile(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "lockFile: checkPathname", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "Errore esito closeFile", esitoAPI)
    PRINT("lockFile eseguita con sueccesso")

    return 0;
}

int unlockFile(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "unlockFile: checkPathname", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "Errore esito unlockFile", esitoAPI)
    PRINT("unlockFile eseguita con sueccesso")

    return 0;
}

int removeFile(const char *pathname)
{
    //controllo argomento
    CS(checkPathname(pathname) == -1, "removeFile: checkPathname", EINVAL)


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
    CS(esitoAPI != API_SUCCESS, "removeFile esito closeFile", esitoAPI)
    PRINT("removeFile eseguita con sueccesso")

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
