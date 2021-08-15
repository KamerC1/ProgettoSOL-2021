#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

#include "commandLine_parser.c" //mettere .h [controllare]
#include "clientApi.c" //mettere .h [controllare]
#include "../strutture_dati/queueChar/queueChar.c" //mettere .h [controllare]

void runRequest(char optionF, char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void tokenString(char stringToToken[], NodoQiPtr_string *testaStringPtr, NodoQiPtr_string *codaStringPtr);
void create_OR_Open_File(char argumentF[]);
int isNumber(const char* s);
char *getAbsPath(char *tempString);
void visitDir(const char stringa[], const char dirnameCache[], int *N);

void gestione_w(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_W(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_r(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_R(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_l(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_c(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);

static bool enableStdout;
#define STAMPA_STDOUT(val) if(enableStdout == true) {val;}

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
    SYSCALL(cmlParsing(&testaCLPtr, &codaCLPtr, argc, argv, &sockName, &enableStdout), "cmlParsing")

    //Gestisce l'opzione -f
    if(sockName != NULL)
    {
        struct timespec abstime;
        abstime.tv_sec = time(NULL) + 10;
        SYSCALL(openConnection(sockName, 1000, abstime), "Errore")

        free(sockName);
    }


    while(testaCLPtr != NULL)
    {
        char option;
        char *argument = NULL;

        SYSCALL(popCoda(&testaCLPtr, &codaCLPtr, &option, &argument), "Errore: popCoda")

        runRequest(option, argument, &testaCLPtr, &codaCLPtr);

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
            PRINT("h") //da implementare
            break;
        case 'f':
            //Operazione ignorata: già gestita altrove
            break;
        case 'w':
//            PRINT("w");
            gestione_w(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'W':
//            PRINT("W");
            gestione_W(argumentF, testaCLPtrF, codaCLPtrF);

            break;
        case 'D':
            PRINT("D");

            break;
        case 'r':
//            PRINT("r");
            gestione_r(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'R':
//            PRINT("R");
            gestione_R(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'd':
            PRINT("d");

            break;
        case 't':
            PRINT("t");
            break;
        case 'l':
//            PRINT("l");
            gestione_l(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'u':
            PRINT("u");
            break;
        case 'c':
//            PRINT("c");
            gestione_c(argumentF, testaCLPtrF, codaCLPtrF);
            break;
        case 'p':
//            PRINT("p");
            break;
        default:
            puts("ERRORE: runRequest");
            exit(EXIT_FAILURE);
            break;
    }
}

//se un file non è stato creato, viene creato.
//Se un file file non è stato aperto, viene aperto
void create_OR_Open_File(char argumentF[])
{
    int isPathPresentVal = isPathPresent(argumentF);
    SYSCALL(isPathPresentVal, "create_OR_Open_File")

    if(isPathPresentVal == NOT_PRESENTE)
    {
        SYSCALL(openFile(argumentF, O_CREATE), "Errore")
    }
    else if(isPathPresentVal == NOT_APERTO)
    {
        SYSCALL(openFile(argumentF, O_OPEN), "Errore")
    }
}

////apre il file identificato da "argumentF" se non è stato già aperto.
//void open_File(char argumentF[])
//{
//    int isPathPresentVal = isPathPresent(argumentF);
//    SYSCALL(isPathPresentVal, "open_File")
//
//    if(isPathPresentVal == NOT_APERTO)
//    {
//        SYSCALL(openFile(argumentF, O_OPEN), "Errore")
//    }
//    if(isPathPresentVal == NOT_PRESENTE)
//    {
//        printf("open_File: Il file %s non è presente nel fileSystem\n", argumentF);
//        exit(EXIT_FAILURE);
//    }
//}

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
    char *dirnameWrite = popString(&testaStringPtr, &codaStringPtr);
    int numFileToWrite = -1;
    if (testaStringPtr != NULL)
    {
        char *numToConvert = popString(&testaStringPtr, &codaStringPtr);
        numFileToWrite = isNumber(numToConvert);
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

    //==Calcola il path dove lavora il processo==
    char *processPath = getcwd(NULL, PATH_MAX);
    size_t pathLength = strlen(processPath); //il +1 è già compreso in MAX_PATH_LENGTH
    REALLOC(processPath, pathLength + 1) //getcwd alloca "MAX_PATH_LENGTH" bytes

    int countFile = numFileToWrite;
    visitDir(dirnameWrite, dirnameCache, &countFile);
    free(dirnameWrite);

    SYSCALL(chdir(processPath), "visitDir: chdir")
    free(processPath);

    if(dirnameCache != NULL)
    {
        free(dirnameCache);
    }

    STAMPA_STDOUT(printf("File letti: %d\n", numFileToWrite - countFile))
    STAMPA_STDOUT(puts("Esito: positivo"))
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


        create_OR_Open_File(realPath);

        //il prossimo opt è -D (quest'ultimo viene rimosso dalla coda)
        if(argument != NULL)
        {
            SYSCALL(writeFile(realPath, argument), "Errore")
        }
        else
        {
            SYSCALL(writeFile(realPath, NULL), "Errore")
        }

        free(realPath);
        tempStringPop = popString(&testaStringPtr, &codaStringPtr);
    }

    if(argument != NULL)
    {
        free(argument);
    }
    assert(testaStringPtr == NULL);
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

        SYSCALL(openFile(argumentF, O_OPEN), "Errore")

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
    printf("N: %d\n", N);

    //il prossimo opt è -d (quest'ultimo viene rimosso dalla coda)
    char *dirname = NULL;
    if(equalToLptr(*testaCLPtrF, 'd'))
    {
        char option;
        popCoda(testaCLPtrF, codaCLPtrF, &option, &dirname);

        SYSCALL(readNFiles(N, dirname), "Errore")
    }
    else
    {
        PRINT("Opzione -d non presente");
    }
    //se non è specificato -d, allora non si fa nulla.
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

        SYSCALL(openFile(argumentF, O_OPEN), "Errore")

        STAMPA_STDOUT(puts("\nOperazione: lockFile"))
        STAMPA_STDOUT(printf("File: %s\n", realPath))
        if(lockFile(realPath) == -1)
        {
            STAMPA_STDOUT(puts("Esito: Negativo"))
            perror("Errore: gestione_c");
            freeQueueStringa(&testaStringPtr, &codaStringPtr);
            free(realPath);
            exit(EXIT_FAILURE);
        }
        STAMPA_STDOUT(puts("Esito: Positivo\n"))

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

        SYSCALL(openFile(argumentF, O_OPEN), "Errore")

        STAMPA_STDOUT(puts("\nOperazione: unlockFile"))
        STAMPA_STDOUT(printf("File: %s\n", realPath))
        if(unlockFile(realPath) == -1)
        {
            STAMPA_STDOUT(puts("Esito: Negativo"))
            perror("Errore: gestione_c");
            freeQueueStringa(&testaStringPtr, &codaStringPtr);
            free(realPath);
            exit(EXIT_FAILURE);
        }
        STAMPA_STDOUT(puts("Esito: Positivo\n"))

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

        SYSCALL(openFile(argumentF, O_OPEN), "Errore")

        STAMPA_STDOUT(puts("\nOperazione: removeFile"))
        STAMPA_STDOUT(printf("File: %s\n", realPath))
        //Non Termina il processo solamente se la remove fallsce perché non non è stato trovato il file
        if(removeFile(realPath) == -1)
        {
            if(errno == ENOENT)
            {
                STAMPA_STDOUT(puts("Esito: File non presente nel server"))
                STAMPA_STDOUT(puts("Esito: Negativo"))
            }
            else
            {
                STAMPA_STDOUT(puts("Esito: Negativo"))
                perror("Errore: gestione_c");
                freeQueueStringa(&testaStringPtr, &codaStringPtr);
                free(realPath);
                exit(EXIT_FAILURE);
            }
        } else
        {
            STAMPA_STDOUT(puts("Esito: Positivo\n"))
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
void visitDir(const char stringa[], const char dirnameCache[], int *N)
{
    if(*N == 0)
        return;

    DIR *directory = opendir(stringa);
    NULL_SYSCALL(directory, "visitDir: opendir")

//    printf("Directory: %s\n", stringa);
    SYSCALL(chdir(stringa), "visitDir: chdir")

    struct dirent *readDirecotry;
    errno = 0;
    while((readDirecotry = readdir(directory)) != NULL)
    {
        if(readDirecotry->d_type != DT_DIR)
        {
            char *realPath = getRealPath(readDirecotry->d_name);
            NULL_SYSCALL(realPath, "Errore gestione_W: getRealPath")
            create_OR_Open_File(realPath);

            //se getSizeFileByte restituisce errore, si ignora il file "realPath"
            if(getSizeFileByte(realPath) == 0)
            {
                *N = *N - 1;
                SYSCALL(writeFile(realPath, dirnameCache), "Errore")
            }
            free(realPath);
        }

        if(*N == 0)
        {
            closedir(directory);
            return;
        }

        if(readDirecotry->d_type == DT_DIR && strncmp(readDirecotry->d_name, "..", 255) != 0 && strncmp(readDirecotry->d_name, ".", 255))
        {
            visitDir(readDirecotry->d_name, dirnameCache, N);
        }

        errno = 0;
    }

    if(errno != 0)
    {
        perror("Impossibile leggere il file/dir");
        exit(EXIT_FAILURE);
    }

    SYSCALL(chdir(".."), "visitDir: chdir")

    closedir(directory);
}