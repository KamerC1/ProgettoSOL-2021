#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

void push(NodoQiPtr *testaPtrF, NodoQiPtr *codaPtrF, int dataF)
{
    NodoQiPtr nuovoPtr = malloc(sizeof(NodoQi));
    if(nuovoPtr != NULL)
    {
        nuovoPtr->data = dataF;
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
    else
    {
        perror("Memoria esaurita");
        exit(EXIT_FAILURE);
    }
}

int pop(NodoQiPtr *lPtrF)
{
    if(*lPtrF != NULL)
    {
        int value = (*lPtrF)->data;
        NodoQiPtr tempPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        free(tempPtr);

        return value;
    }
    else
    {
        puts("la lista è vuota");
        exit(EXIT_FAILURE);
    }
}

int top(NodoQiPtr lPtrF)
{
    if(lPtrF != NULL)
    {
        return lPtrF->data;
    }
    else
    {
        puts("la lista è vuota");
        exit(EXIT_FAILURE);
    }
}

void stampa(NodoQiPtr lPtrF)
{
    if(lPtrF != NULL)
    {
        printf("%d -> ", lPtrF->data);

        stampa(lPtrF->prossimoPtr);
    }
    else
        puts("NULL");
}

void freeLista(NodoQiPtr *lPtrF, NodoQiPtr *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoQiPtr temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        free(temPtr);

        freeLista(lPtrF, codaPtr);
    }
    else
    {
        *codaPtr = NULL;
    }
}