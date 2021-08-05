#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util.h"

struct nodo
{
    char option;
    char *argument;
    struct nodo *prossimoPtr;
};
typedef struct nodo Nodo;
typedef Nodo *NodoPtr;


int pushCoda(NodoPtr *testaPtrF, NodoPtr *codaPtrF, char optionF, char argumentF[]);
int popCoda(NodoPtr *lPtrF, char *optionF, char **argumentF);
int topCoda(NodoPtr lPtrF, char *optionF, char **argumentF);
void stampaCoda(NodoPtr lPtrF);
void freeLista(NodoPtr *lPtrF, NodoPtr *codaPtr);

int main()
{
    NodoPtr testaPtr = NULL;
    NodoPtr codaPtr = NULL;

    SYSCALL(pushCoda(&testaPtr, &codaPtr, 'a', "cane di"), "ERRORE")
    SYSCALL(pushCoda(&testaPtr, &codaPtr, 'b', "gatto da"), "ERRORE")
    SYSCALL(pushCoda(&testaPtr, &codaPtr, 'c', "maiale un"), "ERRORE")
    SYSCALL(pushCoda(&testaPtr, &codaPtr, 'd', "porco tu"), "ERRORE")
    stampaCoda(testaPtr);

    char option;
    char *argument = NULL;
    SYSCALL(popCoda(&testaPtr, &option, &argument), "ERRORE")

    stampaCoda(testaPtr);

    printf("option: %c\n", option);
    printf("argument: %s\n", argument);

    char optionf;
    char *argumentf = NULL;
    SYSCALL(topCoda(testaPtr, &optionf, &argumentf), "ERRORE")
    printf("optionf: %c\n", optionf);
    printf("argumentf: %s\n", argumentf);

    free(argument);
    free(argumentf);
    freeLista(&testaPtr, &codaPtr);

    return 0;
}

int pushCoda(NodoPtr *testaPtrF, NodoPtr *codaPtrF, char optionF, char argumentF[])
{
    NodoPtr nuovoPtr = malloc(sizeof(Nodo));
    CS(nuovoPtr == NULL, "push: malloc(sizeof(Nodo))", errno)

    nuovoPtr->option = optionF;

    //memorizza argumentF
    int argumentFLength = strlen(argumentF);
    nuovoPtr->argument = malloc(argumentFLength+1);
    CSA(nuovoPtr->argument == NULL, "push: malloc(argumentFLength+1)", errno, free(nuovoPtr))
    strncpy(nuovoPtr->argument, argumentF, argumentFLength+1);

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

    return 0;
}

//argumentF deve essere NULL, altrimenti restituisce -1, errno = EPERM
int popCoda(NodoPtr *lPtrF, char *optionF, char **argumentF)
{
    CS(*lPtrF == NULL, "popCoda: la lista è vuota", EPERM)
    CS(*argumentF != NULL, "popCoda: argumentF NON è NULL", EPERM)

    *optionF = (*lPtrF)->option;

    //copio argument in argumentF
    int argumentLength = strlen((*lPtrF)->argument);
    *argumentF = malloc(argumentLength + 1);
    CS(*argumentF == NULL, "popCoda: malloc", errno);
    strncpy(*argumentF, (*lPtrF)->argument, argumentLength + 1);

    NodoPtr tempPtr = *lPtrF;
    *lPtrF = (*lPtrF)->prossimoPtr;
    free(tempPtr->argument);
    free(tempPtr);

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
    *argumentF = malloc(argumentLength + 1);
    CS(*argumentF == NULL, "popCoda: malloc", errno);
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
