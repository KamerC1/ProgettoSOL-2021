#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>


#include "commandLine_parser.c" //mettere .h [controllare]
#include "clientApi.c" //mettere .h [controllare]
#include "../strutture_dati/queueChar/queueChar.c" //mettere .h [controllare]

void runRequest(char optionF, char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void tokenString(char stringToToken[], NodoQiPtr_string *testaStringPtr, NodoQiPtr_string *codaStringPtr);
void create_OR_Open_File(char argumentF[]);
int isNumber(const char* s);
char *getAbsPath(char *tempString);
void visitDir(const char stringa[], const char dirnameCache[], int *N);
int isdot(const char dir[]);

void gestione_a(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_w(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_W(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_r(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_R(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_l(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_u(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_c(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);

bool EN_STDOUT; //mettere in .h di clientAPI

#define HELP_MSG \
"-f filename:  nome del socket a cui connettersi;\n\
-w dirname[,n]: invia al server i file presenti in diraname (tutti oppure fino a n files)\n\
-D dirname: cartella dove memorizzare file espulsi da -w o -W\n\
-r file1,[file2]: nome di file da leggere\n\
-R[n]: legge fino a n file memorizzati nel server (tutti se n=0 o non è specificata)\n\
-d dirname: cartella dove memorizzare file letti con -r o -R\n\
-t time: tempo in msec che intercorre tra l’invio di due richieste successive al server\n\
-l file1[,file2]: file su cui acquisire la mutua esclusione;\n\
-u file1[,file2]: file su cui rilasciare la mutua esclusione;\n\
-c file1[,file2]: file da rimuovere dal server;\n\
-p: abilita stampa sullo stdout per ogni operazione del server"

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        puts("Argomenti mancanti");
        exit(EXIT_FAILURE);
    }

    NodoCLP *testaCLPtr = NULL;
    NodoCLP *codaCLPtr = NULL;

    char *sockName = NULL;
    long timeBeetwenOp = 0;
    SYSCALL(cmlParsing(&testaCLPtr, &codaCLPtr, argc, argv, &sockName, &EN_STDOUT, &timeBeetwenOp), "cmlParsing")

    //Gestisce l'opzione -f
    if(sockName != NULL)
    {
        struct timespec abstime;
        abstime.tv_sec = time(NULL) + 10;
        SYSCALL(openConnection(sockName, 1000, abstime), "Errore")

        free(sockName);
    }

    bool isFirstOpt = true; //vale true se si sta gestendo la prima richiesta
    while(testaCLPtr != NULL)
    {
        char option;
        char *argument = NULL;

        SYSCALL(popCoda(&testaCLPtr, &codaCLPtr, &option, &argument), "Errore: popCoda")


        if(isFirstOpt == false) //se è la pirma richiesta, allora non devo aspettare
        {
            usleep(1000 * timeBeetwenOp);   //Non vale per -p e -f
        }

        runRequest(option, argument, &testaCLPtr, &codaCLPtr);
        isFirstOpt = false;
        if(argument != NULL)
            free(argument);
    }

    SYSCALL(closeConnection(SOCKNAME), "Errore")

    return 0;
}

void runRequest(char optionF, char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    switch (optionF)
    {
        case 'h':
//            PRINT("h")
            puts(HELP_MSG);
            freeCoda(testaCLPtrF, codaCLPtrF); //dopo -h non vengono accettate più richieste
            break;
        case 'f':
            //Operazione ignorata: già gestita altrove
            break;
        case 'a':
            gestione_a(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'w':
//            PRINT("w");
            gestione_w(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'W':
            PRINT("W");
            gestione_W(argumentF, testaCLPtrF, codaCLPtrF);

            break;
        case 'D':
            PRINT("D");

            break;
        case 'r':
            PRINT("r");
            gestione_r(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'R':
//            PRINT("R");
            gestione_R(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'd':
            PRINT("d");

            break;
        case 'l':
//            PRINT("l");
            gestione_l(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'u':
//            PRINT("u");
            gestione_u(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'c':
//            PRINT("c");
            gestione_c(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        default:
            puts("ERRORE: runRequest");
            exit(EXIT_FAILURE);
            break;
    }
}

void tokenString(char stringToToken[], NodoQiPtr_string *testaStringPtr, NodoQiPtr_string *codaStringPtr)
{
    char *savePtr;
    char *token = strtok_r(stringToToken, ",", &savePtr);

    while(token != NULL)
    {
        pushStringa(testaStringPtr, codaStringPtr, token);

        token = strtok_r(NULL, ",", &savePtr);
    }
}

void gestione_w(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //il prossimo opt è -D (quest'ultimo viene rimosso dalla coda)
    char *dirnameCache = NULL;
    if(equalToLptr(*testaCLPtrF, 'D'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &dirnameCache);
    }


    //Faccio la write di ogni elemento di testaStringPtr
    char *dirnameWrite = popString(&testaStringPtr, &codaStringPtr); //se non è stato passato una directory, allora visitDir fallisce
    int numFileToWrite = -1;
    if (testaStringPtr != NULL)
    {
        char *numToConvert = popString(&testaStringPtr, &codaStringPtr);
        numFileToWrite = isNumber(numToConvert); //se viene passato un valore sbagliato, si procede come se non fosse stato passato nessun arg
        free(numToConvert);

        if(numFileToWrite == 0)
            numFileToWrite = -1; //se "numFileToWrite" è un numero negativo, allora vengono letti tutti i file possibili

        if(testaStringPtr != NULL)
        {
            errno = EINVAL;
            PRINT("Sono stati passati troppi argomenti a -w")
            freeQueueStringa(&testaStringPtr, &codaStringPtr);
            exit(EXIT_FAILURE);
        }
    }

    //visito la directory ricorsivamente
    int countFile = numFileToWrite;
    visitDir(dirnameWrite, dirnameCache, &countFile);
    free(dirnameWrite);


    if(dirnameCache != NULL)
    {
        free(dirnameCache);
    }

    STAMPA_STDOUT(printf("File letti: %d\n", numFileToWrite - countFile))
}

void gestione_W(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //il prossimo opt è -D (quest'ultimo viene rimosso dalla coda)
    char *argument = NULL;
    if(equalToLptr(*testaCLPtrF, 'D'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &argument);
    }


    //Faccio la write di ogni elemento di testaStringPtr
    char *tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    while (tempStringPop != NULL)
    {
        //recupero il path assoluto del file. Se il file non esiste, restituisce errore
        char *realPath = getRealPath(tempStringPop);
        NULL_SYSCALL(realPath, "Errore gestione_W: getRealPath")
        free(tempStringPop);


        SYSCALL(openFile(realPath, O_CREATE + O_OPEN), "Erdfdrore")

        SYSCALL(writeFile(realPath, argument), "Errore")

        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    if(argument != NULL)
    {
        free(argument);
    }
    assert(testaStringPtr == NULL);
}

void gestione_a(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    char *savePtr;
    char *tempPathName = strtok_r(argumentF, ",", &savePtr);
    NULL_SYSCALL(tempPathName, "gestione_a: argomento mal formattata")

    size_t lenTempPathName = strlen(tempPathName);
    char *pathName = malloc(sizeof(char) * (lenTempPathName+1)); //serve la malloc perché getAbsPath fa la free
    NULL_SYSCALL(pathName, "gestione_a: malloc")
    memset(pathName, '\0', lenTempPathName+1);
    strncpy(pathName, tempPathName, lenTempPathName);


    char *tempString = strtok_r(NULL, "", &savePtr);
    NULL_SYSCALL(tempString, "gestione_a: argomento mal formattata")

    size_t lenTempString = strlen(tempString);
    char string[lenTempString+1];
    memset(string, '\0', lenTempString+1);
    strncpy(string, tempString, lenTempString);


    //il prossimo opt è -D (quest'ultimo viene rimosso dalla coda)
    char *dirName = NULL;
    if(equalToLptr(*testaCLPtrF, 'D'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &dirName);
    }

    //recupero il path assoluto del file. Se il file non esiste, restituisce errore
    char *realPath = getAbsPath(pathName);
    NULL_SYSCALL(realPath, "Errore gestione_a: getRealPath")

    SYSCALL(openFile(realPath, O_CREATE + O_OPEN), "Erdfdrore")

    int lenString = strlen(string);
    assert(lenString >= 1); //non è possibile avere una stringa vuota
    SYSCALL(appendToFile(realPath, string, lenString+1, dirName), "Errore")

    free(realPath);

    if(dirName != NULL)
    {
        free(dirName);
    }
}

void gestione_r(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //il prossimo opt è -d (quest'ultimo viene rimosso dalla coda)
    char *dirname = NULL;
    if(equalToLptr(*testaCLPtrF, 'd'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &dirname);
    }


    //Faccio la read di ogni elemento di testaStringPtr
    char *tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    while (tempStringPop != NULL)
    {
        //Recupero il path assoluto del file.
        char *realPath = getAbsPath(tempStringPop);
        SYSCALL(openFile(realPath, O_OPEN), "Errore")

        if(dirname != NULL)
        {
            SYSCALL(copyFileToDir(realPath, dirname), "Errore")
        } else
        {
            char *readBuffer = NULL;
            size_t bytesReadBuffer = 0;
            SYSCALL(readFile(realPath, &readBuffer, &bytesReadBuffer), "Errore")
            printf("Contenuto file: \n%s\n", readBuffer);

            if(readBuffer != NULL)
                free(readBuffer);
        }


        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    if(dirname != NULL)
    {
        free(dirname);
    }
    assert(testaStringPtr == NULL);
}

void gestione_R(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    int N = isNumber(argumentF);
    SYSCALL(N, "isNumber")

    //il prossimo opt è -d (quest'ultimo viene rimosso dalla coda)
    char *dirname = NULL;
    if(equalToLptr(*testaCLPtrF, 'd'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &dirname);

        SYSCALL(readNFiles(N, dirname), "Errore")

        free(dirname);
    }
    else
    {
        //se non è specificato -d, allora non si fa nulla.
        PRINT("Opzione -d non presente");
    }
}

//Il file deve essere presente nel file system per poter fare la lock
void gestione_l(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //Faccio la lock di ogni elemento di testaStringPtr
    char *tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    while (tempStringPop != NULL)
    {
        char *realPath = getAbsPath(tempStringPop);

        int isPathPresentVal = isPathPresent(realPath);
        SYSCALL(isPathPresentVal, "gestione_l")

        SYSCALL(openFile(realPath, O_CREATE + O_OPEN), "Erdfdrore")
        SYSCALL(lockFile(realPath), "Errore")


        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    assert(testaStringPtr == NULL);
}

//Il file deve essere presente nel file system per poter fare la unlock
void gestione_u(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //Faccio la lock di ogni elemento di testaStringPtr
    char *tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    while (tempStringPop != NULL)
    {
        char *realPath = getAbsPath(tempStringPop);

        SYSCALL(openFile(realPath, O_OPEN), "Errore")

        if(unlockFile(realPath) == -1)
        {
            perror("Errore: gestione_u");
            freeQueueStringa(&testaStringPtr, &codaStringPtr);
            free(realPath);
            exit(EXIT_FAILURE);
        }

        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    assert(testaStringPtr == NULL);
}

void gestione_c(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF)
{
    NodoQi_string *testaStringPtr = NULL;
    NodoQi_string *codaStringPtr = NULL;

    tokenString(argumentF, &testaStringPtr, &codaStringPtr);

    //Faccio la remove di ogni elemento di testaStringPtr
    char *tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    while (tempStringPop != NULL)
    {
        char *realPath = getAbsPath(tempStringPop);

        SYSCALL(openFile(realPath, O_OPEN), "Errore")

        //Non Termina il processo solamente se la remove fallisce perché non è stato trovato il file
        if(removeFile(realPath) == -1)
        {
            if(errno == ENOENT)
            {
                STAMPA_STDOUT(puts("Esito: File non presente nel server"))
            }
            else
            {
                perror("Errore: gestione_c");
                freeQueueStringa(&testaStringPtr, &codaStringPtr);
                free(realPath);
                exit(EXIT_FAILURE);
            }
        }

        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    assert(testaStringPtr == NULL);
}

//Recupero il path assoluto del file. Termina se il file non esiste su disco e il path passato non è assoluto.
// (Funziona solo su sistemi UNIX)
char *getAbsPath(char *tempString)
{
    char *realPath = NULL;
    if(tempString[0] != '/')
    {
        realPath = getRealPath(tempString);
        NULL_SYSCALL(realPath, "Inserire un path assoluto")
        free(tempString);
    }
    else
    {
        realPath = tempString;
    }

    assert(realPath != NULL);
    return realPath;
}

//data una stringa, restituisce l'intero corrispondente
int isNumber(const char* s)
{
    char* e = NULL;
    errno = 0;
    long val = strtol(s, &e, 0);
    if (e != NULL && *e == (char)0)
    {
        if(val > INT_MAX || val < INT_MIN)
        {
            errno = ERANGE;
            perror("Numero troppo grande o piccolo: ");
            exit(EXIT_FAILURE);
        }

        return (int) val;
    }

    errno = EINVAL;
    return -1;
}

//visita ricorsivamente una directory e, per ogni file, chiama la writeFile
//Se N è un numero negativo, allora vengono letti tutti i file possibili
void visitDir(const char dirName[], const char dirnameCache[], int *N)
{
    if(*N == 0)
        return;

    // controllo che il parametro sia una directory
    struct stat statbuf;
    int returnStat = stat(dirName, &statbuf);
    SYSCALL(returnStat, "visitDir: stat")


    DIR *directory = opendir(dirName);
    NULL_SYSCALL(directory, "visitDir: opendir")


    struct dirent *file;

    while ((errno = 0, file = readdir(directory)) != NULL && *N != 0) //termino se leggo N file
    {
        struct stat statbuf;
        char filename[PATH_MAX];
        int len1 = strlen(dirName);
        int len2 = strlen(file->d_name);
        if ((len1 + len2 + 2) > PATH_MAX)
        {
            fprintf(stderr, "visitDir: PATH_MAX troppo piccolo\n");
            exit(EXIT_FAILURE);
        }
        strncpy(filename, dirName, PATH_MAX - 1);
        strncat(filename, "/", PATH_MAX - 1);
        strncat(filename, file->d_name, PATH_MAX - 1);

        SYSCALL(stat(filename, &statbuf), "visitDir: stat")

        if (S_ISDIR(statbuf.st_mode))
        {
            if (!isdot(filename))
                visitDir(filename, dirnameCache, N);
        }
        else
        {
            //Faccio la write del file

            char *realPath = getRealPath(filename);
            printf("CANE: %s\n", filename);
            NULL_SYSCALL(realPath, "visitDir: getRealPath")
            SYSCALL(openFile(realPath, O_CREATE + O_OPEN), "Erdfdrore")

            //se la write fallisce, non viene contata come scrittura e si procede oltre
            if(writeFile(realPath, dirnameCache) != -1)
            {
                *N = *N - 1;
            }
            free(realPath);
        }
    }

    if(errno != 0)
    {
        perror("Impossibile leggere il file/dir");
        exit(EXIT_FAILURE);
    }

    CLOSEDIR(directory);
}

int isdot(const char dir[])
{
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.'))
        return 1;

    return 0;
}
