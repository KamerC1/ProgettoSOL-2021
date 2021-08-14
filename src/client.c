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
void open_File(char argumentF[]);
int isNumber(const char* s);

void gestione_W(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_r(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);
void gestione_R(char argumentF[], NodoCLPtr *testaCLPtrF, NodoCLPtr *codaCLPtrF);


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
    SYSCALL(cmlParsing(&testaCLPtr, &codaCLPtr, argc, argv, &sockName), "cmlParsing")

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

//bool createFile(const char pathname[])
//{
//    if(hasCreatedFile == true)
//        SYSCALL(openFile(pathname, ))
//}

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
            PRINT("w");
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
            PRINT("l");
            break;
        case 'u':
            PRINT("u");
            break;
        case 'c':
            PRINT("c");
            break;
        case 'p':
            PRINT("p");
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
    SYSCALL(isPathPresentVal, "gestione_W")

    if(isPathPresentVal == NOT_PRESENTE)
    {
        SYSCALL(openFile(argumentF, O_CREATE), "Errore")
    }
    else if(isPathPresentVal == NOT_APERTO)
    {
        SYSCALL(openFile(argumentF, O_OPEN), "Errore")
    }
}

//apre il file identificato da "argumentF" se non è stato già aperto.
void open_File(char argumentF[])
{
    int isPathPresentVal = isPathPresent(argumentF);
    SYSCALL(isPathPresentVal, "gestione_W")

    if(isPathPresentVal == NOT_APERTO)
    {
        SYSCALL(openFile(argumentF, O_OPEN), "Errore")
    }
    if(isPathPresentVal == NOT_PRESENTE)
    {
        printf("Il file %s non è presente nel fileSystem\n", argumentF);
        exit(EXIT_FAILURE);
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
        //recupero il path assoluto del file. Se il file non esiste, restituisce errore
        char *realPath = getRealPath(tempStringPop);
        NULL_SYSCALL(realPath, "gestione_r: getRealPath")
        free(tempStringPop);

        open_File(realPath);

        if(dirname != NULL)
        {
            SYSCALL(copyFileToDir(realPath, dirname), "Errore")
        } else
        {
            char *readBuffer = NULL;
            size_t bytesReadBuffer = 0;
            SYSCALL(readFile(realPath, &readBuffer, &bytesReadBuffer), "Errore")
            printf("Contenuto file: \n%s\n", readBuffer);
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