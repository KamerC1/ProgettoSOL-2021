#include<stdio.h>
#include<stdlib.h>
#include "sortedList.h"

void insertSortedList(NodoSLPtr *lPtr, int valore)
{
    if(*lPtr == NULL)
    {
        NodoSLPtr nuovoNodo = malloc(sizeof(NodoSL));
        if(nuovoNodo != NULL)
        {
            nuovoNodo->dato = valore;
            nuovoNodo->prossimoPtr = NULL;
            *lPtr = nuovoNodo;
        }
        else
        {
            perror("Lista: Memoria esaurita");
            exit(EXIT_FAILURE);
        }
    }
    else if((*lPtr)->dato > valore)
    {
        NodoSLPtr nuovoNodo = malloc(sizeof(NodoSL));
        if(nuovoNodo != NULL)
        {
            nuovoNodo->dato = valore;
            nuovoNodo->prossimoPtr = *lPtr;
            *lPtr = nuovoNodo;
        }
        else
        {
            perror("Lista: Memoria esaurita");
            exit(EXIT_FAILURE);
        }
    }
    else
        insertSortedList(&((*lPtr)->prossimoPtr), valore);


}

int deleteSortedList(NodoSLPtr *lPtr, int val)
{
    if (*lPtr != NULL)
    {
        if (val == (*lPtr)->dato)
        {
            NodoSLPtr tempPtr = *lPtr;
            *lPtr = (*lPtr)->prossimoPtr;
            free(tempPtr);

            return 0;
        }
        else
        {
            NodoSLPtr precedentePtr = *lPtr;
            NodoSLPtr correntePtr = (*lPtr)->prossimoPtr;
            while (correntePtr != NULL && correntePtr->dato != val)
            {
                precedentePtr = correntePtr;
                correntePtr = correntePtr->prossimoPtr;
            }
            if (correntePtr != NULL)
            {
                NodoSLPtr tempPtr = correntePtr;
                precedentePtr->prossimoPtr = correntePtr->prossimoPtr;
                free(tempPtr);

                return 0;
            }
        }
    }
    return -1;
}

int findSortedList(NodoSLPtr lPtr, int valore)
{
    if(lPtr == NULL)
    {
        return -1;
    }
    if(lPtr->dato == valore)
    {
        return 0;
    }
    if(lPtr->dato > valore)
    {
        return -1;
    }
    else
        return findSortedList(lPtr->prossimoPtr, valore);
}

void stampaSortedList(NodoSLPtr lPtr)
{
    if(lPtr != NULL)
    {
        printf("%d -> ", lPtr->dato);
        stampaSortedList(lPtr->prossimoPtr);
    }
    else
        printf("NULL\n");
}

void freeSortedList(NodoSLPtr *lPtrF)
{
    if(*lPtrF != NULL)
    {
        NodoSLPtr temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        free(temPtr);

        freeSortedList(lPtrF);
    }
}