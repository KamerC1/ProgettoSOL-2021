#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../utils/util.h"
#include "queueFile.h"
#include "../sortedList/sortedList.h"


void pushFile(NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *fileF)
{
    NodoQiPtr_File nuovoPtr = malloc(sizeof(NodoQi_file));
    if(nuovoPtr != NULL)
    {
        nuovoPtr->file = fileF;
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

File *popFile(NodoQiPtr_File *lPtrF, NodoQiPtr_File *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoQiPtr_File tempPtr = *lPtrF;
        File *tempFile = tempPtr->file;

        *lPtrF = (*lPtrF)->prossimoPtr;
        if(*lPtrF == NULL)
            *codaPtr = NULL;

        free(tempPtr);
        return tempFile;
    }
    else
    {
        PRINT("NodoQiPtr_File è vuota");
        return NULL;
    }
}

size_t numberOfElements(NodoQiPtr_File lPtrF)
{
    size_t returnValue = 0;
    while(lPtrF != NULL)
    {
        returnValue++;
        lPtrF = lPtrF->prossimoPtr;
    }

    return returnValue;
}

//stampa il path su: "fileLog"
void writePathToFile(NodoQiPtr_File lPtrF, FILE *fileLog)
{
    if(lPtrF != NULL)
    {
        DIE(fprintf(fileLog, "%s\n", lPtrF->file->path))
        writePathToFile(lPtrF->prossimoPtr, fileLog);
    }
}

void freeQueueFile(NodoQiPtr_File *lPtrF, NodoQiPtr_File *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoQiPtr_File temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;
        freeFileData(temPtr->file);
        free(temPtr);

        freeQueueFile(lPtrF, codaPtr);
    }
    else
    {
        *codaPtr = NULL;
    }
}
