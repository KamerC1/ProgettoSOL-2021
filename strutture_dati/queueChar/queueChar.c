#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../utils/util.h"
#include "queueChar.h"

void pushStringa(NodoQiPtr_string *testaPtrF, NodoQiPtr_string *codaPtrF, char dataF[])
{
    NodoQiPtr_string nuovoPtr = malloc(sizeof(NodoQi_string));
    if(nuovoPtr != NULL)
    {
        size_t lengthDataF = strlen(dataF);
        nuovoPtr->stringa = malloc(sizeof(char) * (lengthDataF+1));
        NULL_SYSCALL(nuovoPtr->stringa, "pushStringa: malloc")
        memset(nuovoPtr->stringa, '\0', lengthDataF);
        strncpy(nuovoPtr->stringa, dataF, lengthDataF + 1);

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

size_t max(char primo[], char secondo[])
{
    size_t lenPrimo = strlen(primo);
    size_t lenSec = strlen(secondo);

    if(lenPrimo >= lenSec)
        return lenPrimo;
    else
        return lenSec;
}

//ritorna null se fallisce
char *popString(NodoQiPtr_string *lPtrF, NodoQiPtr_string *codaPtr)
{
    if(*lPtrF != NULL)
    {
        char *tempString = (*lPtrF)->stringa;

        NodoQiPtr_string tempPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        if(*lPtrF == NULL)
            *codaPtr = NULL;

        free(tempPtr);

        return tempString;
    }
    else
    {
//        PRINT("\t\tNodoQiPtr_string è vuota");
        return NULL;
    }
}

//elimina il "data" dalla coda- Ritorna 0 in caso di successo, -1 altrimenti
int deleteDataQueueStringa(NodoQiPtr_string *lPtr, NodoQiPtr_string *codaPtr, char *data)
{
    if (*lPtr != NULL)
    {
        short stringCmp = strncmp((*lPtr)->stringa, data, max((*lPtr)->stringa, data));
        if (stringCmp == 0)
        {

            NodoQiPtr_string tempPtr = *lPtr;
            *lPtr = (*lPtr)->prossimoPtr;

            if(*lPtr == NULL)
            {
                *codaPtr = NULL;
            }

            free(tempPtr->stringa);
            free(tempPtr);

            return 0;
        }
        else
        {
            NodoQiPtr_string precedentePtr = *lPtr;
            NodoQiPtr_string correntePtr = (*lPtr)->prossimoPtr;
            short stringCmp = strncmp(correntePtr->stringa, data, max(correntePtr->stringa, data));
            while (correntePtr != NULL && stringCmp != 0)
            {
                precedentePtr = correntePtr;
                correntePtr = correntePtr->prossimoPtr;
                stringCmp = strncmp(correntePtr->stringa, data, max(correntePtr->stringa, data));
            }
            if (correntePtr != NULL)
            {
                NodoQiPtr_string tempPtr = correntePtr;
                precedentePtr->prossimoPtr = correntePtr->prossimoPtr;

                //Data è posizionato alla fine della coda
                if(*codaPtr == tempPtr)
                {
                    *codaPtr = precedentePtr;
                }

                free(tempPtr->stringa);
                free(tempPtr);

                return 0;
            }
        }
    }
    return -1;
}

void stampaQueueStringa(NodoQiPtr_string lPtrF)
{
    if(lPtrF != NULL)
    {
        printf("%s -> ", lPtrF->stringa);

        stampaQueueStringa(lPtrF->prossimoPtr);
    }
    else
        puts("NULL");
}

void freeQueueStringa(NodoQiPtr_string *lPtrF, NodoQiPtr_string *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoQiPtr_string temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        free(temPtr->stringa);
        free(temPtr);

        freeQueueStringa(lPtrF, codaPtr);
    }
    else
    {
        *codaPtr = NULL;
    }
}


//scambia il primo elemento della coda con il secondo
int swapFirstWithSecond(NodoQiPtr_string *lPtrF, NodoQiPtr_string *codaPtr)
{
    if(*lPtrF == NULL)
        return -1;
    if((*lPtrF)->prossimoPtr == NULL)
        return -1;

    char *temp = (*lPtrF)->stringa;
    (*lPtrF)->stringa = (*lPtrF)->prossimoPtr->stringa;
    (*lPtrF)->prossimoPtr->stringa = temp;

    return 0;
}