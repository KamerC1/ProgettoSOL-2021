#define _POSIX_C_SOURCE 200112L
#include "../utils/util.h"
#include "../utils/conn.h"
#include "../include/serverAPI.h"
#include "../include/cacheFile.h"
#include "../include/log.h"
#include "../include/configParser.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/select.h>
#include <signal.h>


#define SIG_RETURN_SYSCALL(r,c,e) if((r=c)==-1) { if(errno == EINTR) {continue;} else {perror(e);exit(errno);} }
#define SIG_SYSCALL(c,e) if(c==-1) { if(errno == EINTR) {continue;} else {perror(e);exit(errno);} }

//variabile per gestire i segnali
volatile sig_atomic_t terminaOra = 0; //Gestisce SIGINT e SIGQUIT
volatile sig_atomic_t terminaDopo = 0; //Gestisce SIGHUP

#define DEFAULT_LOGFILE "log.txt"

static void sigHandler(int sig)
{
    if(sig == SIGHUP)
    {
        terminaDopo = 1;
    }
    else
        terminaOra = 1;
}

static char *sockName = NULL;

static ServerStorage *storage;

static NodoQi *testaPtr = NULL;
static NodoQi *codaPtr = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t emptyFd = PTHREAD_COND_INITIALIZER;

unsigned int updateMaxSelect(int maxFd, fd_set set);
static void run_server(int pipeW2M_Read);
static void *clientFun(void *pipeW2M_WriteF);
void gestioneCoda(int maxFdF, int fd, fd_set *set);
void cleanup();

void gestioneSegnali();

void gestioneRichiesteAPI(int operazioneAPI, int fdAcceptF);
int gestioneApi_openFile(int fdAcceptF);
long gestioneApi_writeFile(int fdAcceptF);
int gestioneApi_readFile(int fdAcceptF);
long gestioneApi_copyFileToDir(int fdAcceptF);
int gestioneApi_readNFiles(int fdAcceptF, unsigned long long int *bytesLetti);
int gestioneApi_appendToFile(int fdAcceptF);
int gestioneApi_lockFile(int fdAcceptF);
int gestioneApi_unlockFile(int fdAcceptF);
int gestioneApi_closeFile(int fdAcceptF);
int gestioneApi_removeFile(int fdAcceptF);
int gestioneApi_isFilePresent(int fdAcceptF);
int gestioneApi_removeClientInfoAPI(int fdAcceptF);
size_t gestioneApi_getSizeFileByte(int fdAcceptF);

int main(int argc, char *argv[])
{
    ConfigFile_t *configFile = NULL;
    if(argc <= 1)
    {
        puts("File config mancante: Avvio configurazione di default");
        configFile = malloc(sizeof(ConfigFile_t));
        NULL_SYSCALL(configFile, "configParser: malloc")
        setDefaultConfigFile(configFile);
    }
    else
    {
        //se configServer fallisce, non importa perch?? ritorna i valori di default (possono essere parziali o totali)
        configFile = configServer(argv[1]);
    }
    size_t socketNameLen = strlen(configFile->socketName);
    sockName = malloc(sizeof(char) * (socketNameLen+1));
    NULL_SYSCALL(sockName, "Server: malloc")
    strncpy(sockName, configFile->socketName, socketNameLen + 1);

    unlink(sockName);
    atexit(cleanup);

    puts("--------Stampa config------------");
    printConfig(*configFile);
    puts("--------fine config------------");

    gestioneSegnali();

    storage = createStorage(configFile->maxStorageBytes, configFile->maxStorageFiles, configFile->fileReplacementAlg);

    //pu?? sovrascrivere eventuali logFile gi?? presenti
    storage->logFile = fopen(configFile->logFile, "w");
    if(storage->logFile == NULL)
    {
        PRINT("Pathname del log file sbagliato: creo un file log.txt nella directory corrente");
        storage->logFile = fopen(DEFAULT_LOG, "w");
    }
    //storage->logFile = freopen(NULL, "a", storage->logFile);

    int pipeW2M[2]; //gestisce comunicazione tra worker e manager
    SYSCALL(pipe(pipeW2M), "Errore: pipe(pipeW2M)")

    size_t N_THREADS = configFile->numWorker;
    freeConfigFile(configFile);

    pthread_t threadFd[N_THREADS];
    for(int i = 0; i < N_THREADS; i++)
    {
        THREAD_CREATE(&threadFd[i], NULL, &clientFun, (void *) &pipeW2M[WRITE_END], "Thread setId")
    }

    run_server(pipeW2M[READ_END]);


    for(int i = 0; i < N_THREADS; i++)
    {
        THREAD_JOIN(threadFd[i], NULL, "Impossibile fare la join: seetId")
    }

    SYSCALL(close(pipeW2M[WRITE_END]), "Errore: close(pipeW2M[WRITE_END])")
    SYSCALL(close(pipeW2M[READ_END]), "Errore: close(pipeW2M[READ_END])")

    //stampaHash(storage);
    stampaStorage(storage);

    FCLOSE(storage->logFile)
    freeStorage(storage);
    freeLista(&testaPtr, &codaPtr);
    PRINT("\nTermina processo");


    return 0;
}

void gestioneSegnali()
{
    //ignoro SIGPIPE
    sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

    //i segnali da gestire vengono ignorati finch?? l'handler non ?? pronto
    sigset_t mask, oldmask;
    SYSCALL(sigemptyset(&mask), "Errore: sigemptyset(&mask)")
    SYSCALL(sigaddset(&mask, SIGINT), "Errore: sigaddset(&mask, SIGINT)")
//    SYSCALL(sigaddset(&mask, SIGTSTP), "Errore: sigaddset(&mask, SIGTERM)")
    SYSCALL(sigaddset(&mask, SIGQUIT), "Errore: sigaddset(&mask, SIGQUIT)")
    SYSCALL(sigaddset(&mask, SIGHUP), "Errore: sigaddset(&mask, SIGHUP)")
    SYSCALL_NOTZERO(pthread_sigmask(SIG_BLOCK, &mask, &oldmask), "Errore: Spthread_sigmask(SIG_BLOCK, &mask, &oldmask)")


    //GESTIONE SIGINT
    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct)); //inizializza a 0 sSigAction
    sigAct.sa_handler = sigHandler; //registra l'handler

    SYSCALL(sigaction(SIGINT, &sigAct, NULL), "Errore sigaction(SIGINT, &sigAct, NULL)")
//    SYSCALL(sigaction(SIGTSTP, &sigAct, NULL), "Errore sigaction(SIGINT, &sigAct, NULL)")
    SYSCALL(sigaction(SIGQUIT, &sigAct, NULL), "Errore sigaction(SIGINT, &sigAct, NULL)")
    SYSCALL(sigaction(SIGHUP, &sigAct, NULL), "Errore sigaction(SIGINT, &sigAct, NULL)")

    //ripristino la maschera
    SYSCALL_NOTZERO(pthread_sigmask(SIG_SETMASK, &oldmask, NULL), "Errore: Spthread_sigmask(SIG_BLOCK, &mask, &oldmask)")
}

//Thread manager che gestisce i worker
static void run_server(int pipeW2M_Read)
{
    //Socket di connessione
    int fdSkt;
    RETURN_SYSCALL(fdSkt, socket(AF_UNIX, SOCK_STREAM, 0), "Errore creazione socket - fdSkt")
    struct sockaddr_un sckAddr;
    strncpy(sckAddr.sun_path, sockName, MAXBACKLOG);
    sckAddr.sun_family = AF_UNIX;
    SYSCALL(bind(fdSkt, (struct sockaddr *) &sckAddr, sizeof(sckAddr)), "Errore bind - fdSkt")
    SYSCALL(listen(fdSkt, SOMAXCONN), "Errore listen - fdSkt")

    //Massimo fd attivo
    int maxFd = fdSkt;

    //Inizializzazione set
    fd_set set, readSet;
    FD_ZERO(&set);
    FD_SET(fdSkt, &set); //FD_SET imposta a 1 il bit corrispondente a fdSkt
    FD_SET(pipeW2M_Read, &set);
    if(pipeW2M_Read > maxFd)
    {
        maxFd = pipeW2M_Read;
    }

    size_t activeClient = 0;
    int fdSkt_accept;
    while(terminaOra == 0)
    {
        //Termino il server
        if(terminaDopo == 1 && activeClient <= 0)
        {
            assert(activeClient == 0);
            terminaOra = 1;
            BCAST(&emptyFd)
            SYSCALL(close(fdSkt), "Errore close - fdSkt")
            return;
        }

        readSet = set;
        SIG_SYSCALL(select(maxFd + 1, &readSet, NULL, NULL, NULL), "select(fd_num + 1, &rdset, NULL, NULL, NULL)")
        for(int i = 0; i <= maxFd; i++)
        {
            //se in readn, errno = EINTR, allora l'esecuzione ricomincia dal for (o dal while se i > maxFd)
            if(terminaOra == 1 || (terminaDopo == 1 && activeClient <= 0))
            {
                terminaOra = 1;
                BCAST(&emptyFd)
                SYSCALL(close(fdSkt), "Errore close - fdSkt")
                return;
            }


            if (FD_ISSET(i, &readSet))
            {
                //arriva una nuova connessione
                if (i == fdSkt)
                {
                    int esitoConnessione = CONNESSIONE_ACCETTATA;
                    RETURN_SYSCALL(fdSkt_accept, accept(fdSkt, NULL, 0), "fdSkt_accept = accept(fdSkt, NULL, 0)")

                    char *timeString = getCurrentTime();
                    DIE(fprintf(storage->logFile, "Nuova connessione: %d\tData: %s\n\n", fdSkt_accept, timeString))
                    free(timeString);

                    //Non possono accettare nuove connessioni
                    if(terminaDopo == 1)
                    {
                        //notifico il client che la richiesta di connessione ha fallito
                        esitoConnessione = CONNESSIONE_RIFIUTATA;
                        WRITEN_C(fdSkt_accept, &esitoConnessione, sizeof(int), "Erroer: writen(fdSkt_accept)")
                        if(errno == ECONNRESET || errno == EPIPE) //connessione chiusa dal client
                        {
                            continue;
                        }

                        SYSCALL(close(fdSkt_accept), "Errore close - fdSkt_accept")
                        puts("Non accetto nuovo connessioni");
                        continue;
                    }

                    //notifico il client che la richiesta di connessione ha avuto successo
                    WRITEN_C(fdSkt_accept, &esitoConnessione, sizeof(int), "Erroer: writen(fdSkt_accept)")
                    if(errno == ECONNRESET || errno == EPIPE) //connessione chiusa dal client
                    {
                        continue;
                    }

                    activeClient++;
                    FD_SET(fdSkt_accept, &set);
                    if (fdSkt_accept > maxFd) {
                        maxFd = fdSkt_accept;
                    }
                    continue;
                }

                //La select pu?? ri-metteresi in ascolto del worker 'i' perch??, quest'ultimo, ha finito di gestire la richiesta.
                if(i == pipeW2M_Read)
                {
                    int pipeFdSoccket;
                    SYSCALL(readn(pipeW2M_Read, &pipeFdSoccket, sizeof(int)), "Errore")

                    //il client ha chiuso la connessione
                    if(pipeFdSoccket == -1)
                    {
                        char *timeString = getCurrentTime();
                        DIE(fprintf(storage->logFile, "connessione chiusa: %d\tData: %s\n\n", fdSkt_accept, timeString))
                        free(timeString);

                        activeClient--;
                        if(activeClient == 0 && terminaDopo == 1)
                        {
                            terminaOra = 1;
                            BCAST(&emptyFd)
                            SYSCALL(close(fdSkt), "Errore close - fdSkt")
                            return;
                        }
                    }
                    else
                    {
                        FD_SET(pipeFdSoccket, &set);
                        if(pipeFdSoccket > maxFd)
                            maxFd = pipeFdSoccket;
                    }

                    continue;
                }

                //gestisce nuova richiesta dal client
                LOCK(&mutex)
                push(&testaPtr, &codaPtr, i);
                SIGNAL(&emptyFd)
                UNLOCK(&mutex)

                FD_CLR(i, &set);
                if(i == maxFd)
                    maxFd = updateMaxSelect(i, set);
            }
        }
    }
    BCAST(&emptyFd)

    SYSCALL(close(fdSkt), "Errore close - fdSkt")
}

//Thread worker
static void *clientFun(void *pipeW2M_WriteF)
{
    int pipeW2M_Write = *((int *) pipeW2M_WriteF);

    while (terminaOra == 0)
    {
        LOCK(&mutex)

        while (testaPtr == NULL)
        {
            WAIT(&emptyFd, &mutex)

            if(terminaOra == 1)
            {
                UNLOCK(&mutex)
                return NULL;
            }

        }
        int fdAccept = pop(&testaPtr, &codaPtr);
//        printf("Fd in thread: %d\n\n", fdAccept);
        UNLOCK(&mutex)

        //leggo l'operazione da svolgere
        int operazioneAPI;
        int lenghtRead;
        RETURN_SYSCALL(lenghtRead, readn(fdAccept, &operazioneAPI, sizeof(int)), "Errore: read(fdSkt_com, buffer, DIM_BUFFER)")
        //Il client ha chiuso la connessione:
        if(lenghtRead == 0)
        {
            SYSCALL(removeClientInfo(storage, fdAccept), "Errore"); //Termino tutto perch?? il file system potrebbe essere in uno stato instabile
            SYSCALL(close(fdAccept), "Errore close - fdSkt")
//            stampaHash(storage);

            //notifico il manager che la connessione ?? terminata - server per gestire SIGHUP
            int esitoNegativo = -1;
            SYSCALL(writen(pipeW2M_Write, &esitoNegativo, sizeof(int)), "Erroer: writen(pipeW2M_Write, &fdAccept, sizeof(int))")
            continue;
        }

        gestioneRichiesteAPI(operazioneAPI, fdAccept);

        //Notifico il manager di rimettere in ascolto il thread corrente
        SYSCALL(writen(pipeW2M_Write, &fdAccept, sizeof(int)), "Erroer: writen(pipeW2M_Write, &fdAccept, sizeof(int))")
        puts("\n------------Esco------------\n");
    }

    return NULL;
}

//sceglie le operazioni da svolgere in base alla richiesta ricevuta
void gestioneRichiesteAPI(int operazioneAPI, int fdAcceptF)
{
    switch (operazioneAPI) {
        case API_OPENFILE:
            printf("Operazione: %d\n", operazioneAPI);
            int esito = gestioneApi_openFile(fdAcceptF);

            //se esito == -2, significa che ?? in attesa della lock, quindi si rimane in attesa
            if(esito != -2)
            {
                //invia eisto della richiesta (successo o insuccesso)
                WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_OPENFILE: WRITEN_C")
            }

            break;
        case API_WRITEFILE:
            printf("Operazione: %d\n", operazioneAPI);
            long returnValueWrite = gestioneApi_writeFile(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_WRITEFILE: WRITEN_C")
            if (errno == 0)
            {
                WRITEN_C(fdAcceptF, &returnValueWrite, sizeof(long), "API_WRITEFILE: WRITEN_C")
            }

            break;

        case API_READNFILES:
            printf("Operazione: %d\n", operazioneAPI);
            unsigned long long int bytesLetti = 0;
            int returnValueReadN = gestioneApi_readNFiles(fdAcceptF, &bytesLetti);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_READNFILES: writen()")
            if (errno == 0)
            {
                WRITEN_C(fdAcceptF, &bytesLetti, sizeof(unsigned long long int), "API_READNFILES: writen()")
                WRITEN_C(fdAcceptF, &returnValueReadN, sizeof(int), "API_READNFILES: writen()")
            }
            break;

        case API_READFILE:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_readFile(fdAcceptF);
            //la notitifca dell'esito viene fatta in gestioneApi_readFile

            break;

        case API_APPENDTOFILE:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_appendToFile(fdAcceptF);

            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_APPENDTOFILE: writen()")

            break;

        case API_LOCKFILE:
            printf("Operazione: %d\n", operazioneAPI);
            int esitoLock = gestioneApi_lockFile(fdAcceptF);
            //se esito == -2, significa che ?? in attesa della lock
            if(esitoLock != -2)
            {
                //invia eisto della richiesta (successo o insuccesso)
                WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_OPENFILE: WRITEN_C")
            }
            break;

        case API_UNLOCKFILE:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_unlockFile(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_UNLOCKFILE: writen()")
            break;

        case API_CLOSEFILE:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_closeFile(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_CLOSEFILE: writen()")
            break;

        case API_REMOVEFILE:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_removeFile(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_REMOVEFILE: writen()")
            break;

        case API_REMOVE_CLIENT_INFO:
            printf("Operazione: %d\n", operazioneAPI);
            gestioneApi_removeClientInfoAPI(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_REMOVE_CLIENT_INFO: writen()")
            break;

        case API_ISFILE_PRESENT:
            printf("Operazione: %d\n", operazioneAPI);
            int esitoAPI = gestioneApi_isFilePresent(fdAcceptF);
            WRITEN_C(fdAcceptF, &esitoAPI, sizeof(int), "API_ISFILE_PRESENT: writen()")
            break;

        case API_COPY_FILE_TODIR:
            printf("Operazione: %d\n", operazioneAPI);
            long returnValueCopyFileToDir = gestioneApi_copyFileToDir(fdAcceptF);
            WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_WRITEFILE: writen()")
            if(errno == 0)
                WRITEN_C(fdAcceptF, &returnValueCopyFileToDir, sizeof(long), "API_WRITEFILE: writen()")
            break;

        case API_GET_FILE_SIZEBYTE:
            printf("Operazione: %d\n", operazioneAPI);
            size_t returnValue = gestioneApi_getSizeFileByte(fdAcceptF);
            WRITEN_C(fdAcceptF, &returnValue, sizeof(size_t), "API_WRITEFILE: writen()")
            break;

        default:
            puts("gestioneRichiesteAPI: default");
            break;
    }

    //stampaHash(storage);
//    stampaStorage(storage);

}


//Gestisce la richiesta dell'API "openFile" e chiama openFileServer
int gestioneApi_openFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes = 0;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //Leggo la flag
    int flags = 0;
    READN(fdAcceptF, &flags, sizeof(int), "gestioneApi_openFile: read()")
    printf("flags: %d\n", flags);


    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client

    return openFileServer(pathname, flags, storage, fdAcceptF);
}

//Gestisce la richiesta dell'API "readFile" e chiama readFileServer
int gestioneApi_readFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    char *buf = NULL; //la malloc viene fatta in readFileServer
    size_t size;

    //ChiamataAPI - in caso di fallimento, non c'?? bisogno di fare free(buf).
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    long returnValueRead = readFileServer(pathname, &buf, &size, storage, fdAcceptF);
    if(returnValueRead == -1)
    {
        //Notifica esito operazione di readFileServer ed esco
        WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_WRITEFILE: writen()")
        return -1;
    }

    //Notifica esito operazione di readFileServer
    WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_WRITEFILE: writen()")
    if(errno == ECONNRESET || errno == EPIPE)
        return -1;

    //Invio size e buf (in caso di fallimento, devo liberare buf)
    CSA(writen(fdAcceptF, &size, sizeof(size_t)) == -1, "gestioneApi_readFile: writen", errno, if(buf != NULL) free(buf))
    if(buf != NULL)
    {
        CSA(writen(fdAcceptF, buf, size) == -1, "gestioneApi_readFile: writen", errno, free(buf))
        free(buf);
    }

    //Notifica esito operazione di readFileServer
    WRITEN_C(fdAcceptF, &errno, sizeof(int), "API_WRITEFILE: writen()")
    if(errno == ECONNRESET || errno == EPIPE)
        return -1;
    WRITEN_C(fdAcceptF, &returnValueRead, sizeof(long), "API_WRITEFILE: writen()")
    if(errno == ECONNRESET || errno == EPIPE)
        return -1;


    return 0;
}

//Gestisce la richiesta dell'API "copyFileToDir" e chiama copyFileToDirServer
long gestioneApi_copyFileToDir(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_copyFileToDir: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_copyFileToDir: read()")
    printf("pathname: %s\n", pathname);


    //leggo la dimensione della dir
    int dirBytes;
    READN(fdAcceptF, &dirBytes, sizeof(int), "gestioneApi_copyFileToDir: read()")

    //leggo dirname
    char dirname[dirBytes];
    memset(dirname, '\0', dirBytes);
    READN(fdAcceptF, dirname, dirBytes, "gestioneApi_copyFileToDir: read()")
    printf("dirname: %s\n", dirname);

    errno = 0; //errno potrebbe essere stato settato da un'altro client

    long returnValue = copyFileToDirServer(pathname, dirname, storage, fdAcceptF);
    CS(returnValue == -1, "", errno)

    return returnValue;
}

//Gestisce la richiesta dell'API "readNFiles" e chiama readNFileServer
int gestioneApi_readNFiles(int fdAcceptF, unsigned long long int *bytesLetti)
{
    //leggo N
    int N;
    READN(fdAcceptF, &N, sizeof(int), "gestioneApi_openFile: read()")

    //Leggo la dimensione di dirname
    int dirBytes;
    READN(fdAcceptF, &dirBytes, sizeof(int), "gestioneApi_copyFileToDir: read()")

    //leggo dirname
    char dirname[dirBytes];
    memset(dirname, '\0', dirBytes);
    READN(fdAcceptF, dirname, dirBytes, "gestioneApi_copyFileToDir: read()")
    printf("dirname: %s\n", dirname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    int returnValue = readNFilesServer(N, dirname, storage, fdAcceptF, bytesLetti);
    CS(returnValue == -1, "", errno)

    return returnValue;
}

//Gestisce la richiesta dell'API "writeFile" e chiama writeFileServer
long gestioneApi_writeFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_writeFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_writeFile: read()")
    printf("pathname: %s\n", pathname);


    //leggo la dimensione della dir
    int dirBytes = 0;
    READN(fdAcceptF, &dirBytes, sizeof(int), "gestioneApi_writeFile: read()")

    long returnValueWrite = -1;
    if(dirBytes == ARGUMENT_NULL)
    {
        //dir ?? NULL
        errno = 0; //errno potrebbe essere stato settato da un'altro client
        returnValueWrite = writeFileServer(pathname, NULL, storage, fdAcceptF);

        CS(returnValueWrite == -1, "", errno)
    }
    else
    {
        char dirname[dirBytes];
        memset(dirname, '\0', dirBytes);
        READN(fdAcceptF, dirname, dirBytes, "gestioneApi_writeFile: read()")
        printf("dirname: %s\n", dirname);

        errno = 0; //errno potrebbe essere stato settato da un'altro client
        returnValueWrite = writeFileServer(pathname, dirname, storage, fdAcceptF);
        CS(returnValueWrite == -1, "", errno)
    }

    return returnValueWrite;
}

//Gestisce la richiesta dell'API "appendToFile" e chiama appendToFileServer
int gestioneApi_appendToFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_appendToFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_appendToFile: read()")
    printf("pathname: %s\n", pathname);

    //Leggo size
    size_t size;
    READN(fdAcceptF, &size, sizeof(size_t), "gestioneApi_appendToFile: read()")
    char buf[size]; //seize comprende il '\0'
    memset(buf, '\0', size);
    READN(fdAcceptF, buf, size, "gestioneApi_appendToFile: read()")


    //leggo la dimensione della dir
    int dirBytes;
    READN(fdAcceptF, &dirBytes, sizeof(int), "gestioneApi_appendToFile: read()")
    if(dirBytes == ARGUMENT_NULL)
    {
        //dir ?? NULL
        errno = 0; //errno potrebbe essere stato settato da un'altro client
        CS(appendToFileServer(pathname, buf, size, NULL, storage, fdAcceptF) == -1, "", errno)
    }
    else
    {

        char dirname[dirBytes];
        memset(dirname, '\0', dirBytes);
        READN(fdAcceptF, dirname, dirBytes, "gestioneApi_appendToFile: read()")
        printf("dirname: %s\n", dirname);


        errno = 0; //errno potrebbe essere stato settato da un'altro client
        CS(appendToFileServer(pathname, buf, size, dirname, storage, fdAcceptF) == -1, "", errno)
    }

    return 0;
}

//Gestisce la richiesta dell'API "lockFIle" e chiama lockFileServer
int gestioneApi_lockFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    return lockFileServer(pathname, storage, fdAcceptF);
}

//Gestisce la richiesta dell'API "unlockFile" e chiama unlockFileServer
int gestioneApi_unlockFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_unlockFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_unlockFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    CS(unlockFileServer(pathname, storage, fdAcceptF) == -1, "", errno)
    return 0;
}

//Gestisce la richiesta dell'API "closeFile" e chiama closeFileServer
int gestioneApi_closeFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    CS(closeFileServer(pathname, storage, fdAcceptF) == -1, "", errno)
    return 0;
}

//Gestisce la richiesta dell'API "removeFile" e chiama removeFileServer
int gestioneApi_removeFile(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    CS(removeFileServer(pathname, storage, fdAcceptF) == -1, "", errno)
    return 0;
}

//Gestisce la richiesta dell'API "isFilePresent" e chiama isFilePresentServer
int gestioneApi_isFilePresent(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    return isPathPresentServer(pathname, storage, fdAcceptF);
}

//Gestisce la richiesta dell'API "getSizeFileByte" e chiama getSizeFileByteServer
size_t gestioneApi_getSizeFileByte(int fdAcceptF)
{
    //leggo la dimensione del pathname
    int pathnameBytes;
    READN(fdAcceptF, &pathnameBytes, sizeof(int), "gestioneApi_openFile: read()")

    //leggo il pathname
    char pathname[pathnameBytes];
    memset(pathname, '\0', pathnameBytes);
    READN(fdAcceptF, pathname, pathnameBytes, "gestioneApi_openFile: read()")
    printf("pathname: %s\n", pathname);

    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    return getSizeFileByteServer(pathname, storage, fdAcceptF);
}

int gestioneApi_removeClientInfoAPI(int fdAcceptF)
{
    //ChiamataAPI
    errno = 0; //errno potrebbe essere stato settato da un'altro client
    CS(removeClientInfo(storage, fdAcceptF) == -1, "", errno)
    return 0;
}

unsigned int updateMaxSelect(int maxFd, fd_set set)
{
    for(int i = maxFd - 1; i >= 0; i--)
    {
        if(FD_ISSET(i, &set))
        {
            return i;
        }
    }
    return -1;
}

void cleanup()
{
    if(sockName != NULL)
    {
        unlink(sockName);
        free(sockName);
    }
}