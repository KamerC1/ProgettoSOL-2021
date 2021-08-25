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

int pop(NodoQiPtr *lPtrF, NodoQiPtr *codaPtr)
{
    if(*lPtrF != NULL)
    {
        int value = (*lPtrF)->data;
        NodoQiPtr tempPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        if(*lPtrF == NULL)
            *codaPtr = NULL;

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

//ritorna 1 se trova data, 0 altrimenti
int findDataQueue(NodoQiPtr lPtr, int data)
{
    if(lPtr == NULL)
    {
        return 0;
    }
    if(lPtr->data == data)
    {
        return 1;
    }
    else
        return findDataQueue(lPtr->prossimoPtr, data);
}

//elimina il "data" dalla coda- Ritorna 0 in caso di successo, -1 altrimenti
int deleteDataQueue(NodoQiPtr *lPtr, NodoQiPtr *codaPtr, int data)
{
    if (*lPtr != NULL)
    {
        if (data == (*lPtr)->data)
        {

            NodoQiPtr tempPtr = *lPtr;
            *lPtr = (*lPtr)->prossimoPtr;

            if(*lPtr == NULL)
            {
                *codaPtr = NULL;
            }

            free(tempPtr);

            return 0;
        }
        else
        {
            NodoQiPtr precedentePtr = *lPtr;
            NodoQiPtr correntePtr = (*lPtr)->prossimoPtr;
            while (correntePtr != NULL && correntePtr->data != data)
            {
                precedentePtr = correntePtr;
                correntePtr = correntePtr->prossimoPtr;
            }
            if (correntePtr != NULL)
            {
                NodoQiPtr tempPtr = correntePtr;
                precedentePtr->prossimoPtr = correntePtr->prossimoPtr;

                //Data è posizionato alla fine della coda
                if(*codaPtr == tempPtr)
                {
                    //Data è posizionato alla fine della coda
                    *codaPtr = precedentePtr;
                }

                free(tempPtr);

                return 0;
            }
        }
    }
    return -1;
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