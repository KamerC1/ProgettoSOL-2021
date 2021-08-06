#include<stdio.h>
#include<stdlib.h>
#include "sortedList.h"

void insertSortedList(NodoPtr *lPtr, int valore)
{
    if(*lPtr == NULL)
    {
        NodoPtr nuovoNodo = malloc(sizeof(Nodo));
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
        NodoPtr nuovoNodo = malloc(sizeof(Nodo));
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

int deleteSortedList(NodoPtr *lPtr, int val)
{
    if (*lPtr != NULL)
    {
        if (val == (*lPtr)->dato)
        {
            NodoPtr tempPtr = *lPtr;
            *lPtr = (*lPtr)->prossimoPtr;
            free(tempPtr);

            return 0;
        }
        else
        {
            NodoPtr precedentePtr = *lPtr;
            NodoPtr correntePtr = (*lPtr)->prossimoPtr;
            while (correntePtr != NULL && correntePtr->dato != val)
            {
                precedentePtr = correntePtr;
                correntePtr = correntePtr->prossimoPtr;
            }
            if (correntePtr != NULL)
            {
                NodoPtr tempPtr = correntePtr;
                precedentePtr->prossimoPtr = correntePtr->prossimoPtr;
                free(tempPtr);

                return 0;
            }
        }
    }
    puts("Elemento non eliminato dalla lista");
    return -1;
}

int findSortedList(NodoPtr lPtr, int valore)
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
        findSortedList(lPtr->prossimoPtr, valore);
}

void stampaSortedList(NodoPtr lPtr)
{
    if(lPtr != NULL)
    {
        printf("%d -> ", lPtr->dato);
        stampaSortedList(lPtr->prossimoPtr);
    }
    else
        printf("NULL\n");
}

void freeSortedList(NodoPtr *lPtrF)
{
    if(*lPtrF != NULL)
    {
        NodoPtr temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        free(temPtr);

        freeSortedList(lPtrF);
    }
}
