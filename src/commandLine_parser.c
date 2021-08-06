#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "../utils/util.h"
#include "../include/commandLine_parser.h"

void cmlParsing(NodoPtr *testaPtr, NodoPtr *codaPtr, int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, ":hf:w:W:D:r:R::d:l:u:c:p")) != -1)
    {
        switch (opt)
        {
            case 'h':
                if(findOption(*testaPtr, opt) == 0)
                    pushCoda(testaPtr, codaPtr, opt, NULL);
                else PRINT("-h già prsente - ignorato")
                break;
            case 'f':
                if(findOption(*testaPtr, opt) == 0)
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                else PRINT("-f già prsente - ignorato")
                break;
            case 'w':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'W':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'D':
                if(equalTolastOpt(*codaPtr, 'W') == 1)
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                else
                    puts("-D non segue -W");

                break;
            case 'r':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'R':
                printf("%c\t%s\n", opt, optarg);
                if(optarg == NULL)
                    pushCoda(testaPtr, codaPtr, opt, "0");
                break;
            case 'd':
                if(equalTolastOpt(*codaPtr, 'r')|| equalTolastOpt(*codaPtr, 'R'))
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                else
                    puts("-d non segue -r o -R");

                break;
            case 't':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'l':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'u':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'c':
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'p':
                if(findOption(*testaPtr, opt) == 0)
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                else PRINT("-p già prsente - ignorato")
                break;
            case ':': {
                printf("l'opzione '-%c' richiede un argomento\n", optopt);
            }
                break;
            case '?': {  // restituito se getopt trova una opzione non riconosciuta
                printf("l'opzione '-%c' non e' gestita\n", optopt);
            }
                break;
            default:;
        }
    }
}

void pushCoda(NodoPtr *testaPtrF, NodoPtr *codaPtrF, char optionF, char argumentF[])
{
    NodoPtr nuovoPtr;
    RETURN_NULL_SYSCALL(nuovoPtr, malloc(sizeof(Nodo)), "push: malloc(sizeof(Nodo))")


    nuovoPtr->option = optionF;

    //memorizza argumentF
    if(argumentF != NULL)
    {
        int argumentFLength = strlen(argumentF);
        RETURN_NULL_SYSCALL(nuovoPtr->argument, malloc(argumentFLength+1), "push: malloc(argumentFLength+1)")
        strncpy(nuovoPtr->argument, argumentF, argumentFLength+1);
    }

    nuovoPtr->prossimoPtr = NULL;

    if(*testaPtrF == NULL)
    {
        *testaPtrF = nuovoPtr;
        *codaPtrF = nuovoPtr;
    }
    else
    {
        (*codaPtrF)->prossimoPtr = nuovoPtr;
        *codaPtrF = nuovoPtr;
    }
}

//argumentF deve essere NULL, altrimenti restituisce -1, errno = EPERM
int popCoda(NodoPtr *lPtrF, char *optionF, char **argumentF)
{
    CS(*lPtrF == NULL, "popCoda: la lista è vuota", EPERM)
    CS(*argumentF != NULL, "popCoda: argumentF NON è NULL", EPERM)

    *optionF = (*lPtrF)->option;

    //copio argument in argumentF
    int argumentLength = strlen((*lPtrF)->argument);
    RETURN_NULL_SYSCALL(*argumentF, malloc(argumentLength + 1), "popCoda: malloc")
    strncpy(*argumentF, (*lPtrF)->argument, argumentLength + 1);

    NodoPtr tempPtr = *lPtrF;
    *lPtrF = (*lPtrF)->prossimoPtr;
    free(tempPtr->argument);
    free(tempPtr);

    return 0;
}

//ritorna 1 se trova opt, 0 altrimenti
int findOption(NodoPtr lPtr, char opt)
{
    if(lPtr == NULL)
    {
        return 0;
    }
    if(lPtr->option == opt)
    {
        return 1;
    }
    else
        findOption(lPtr->prossimoPtr, opt);
}

//restituisce 1 se l'ultimo elemento della coda è uguale a opt, 0 altrimenti
int equalTolastOpt(NodoPtr codaPtr, char opt)
{
    if(codaPtr == NULL)
    {
        PRINT("equalTolastOpt: coda vuota")
        return 0;
    }

    if(codaPtr->option == opt)
        return 1;

    return 0;
}


//argumentF deve essere NULL, altrimenti restituisce -1, errno = EPERM
int topCoda(NodoPtr lPtrF, char *optionF, char **argumentF)
{
    CS(lPtrF == NULL, "topCoda: la lista è vuota", EINTR)
    CS(*argumentF != NULL, "popCoda: argumentF NON è NULL", EPERM)

    *optionF = lPtrF->option;

    //copio argument in argumentF
    int argumentLength = strlen(lPtrF->argument);
    RETURN_NULL_SYSCALL(*argumentF, malloc(argumentLength + 1), "topCoda: malloc")
    strncpy(*argumentF, lPtrF->argument, argumentLength + 1);

    return 0;
}

void stampaCoda(NodoPtr lPtrF)
{
    if(lPtrF != NULL)
    {
        printf("[option: %c argument: %s]\n", lPtrF->option, lPtrF->argument);

        stampaCoda(lPtrF->prossimoPtr);
    }
    else
        puts("NULL");
}

void freeLista(NodoPtr *lPtrF, NodoPtr *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoPtr temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;

        free(temPtr->argument);
        free(temPtr);

        freeLista(lPtrF, codaPtr);
    }
    else
    {
        *codaPtr = NULL;
    }
}
