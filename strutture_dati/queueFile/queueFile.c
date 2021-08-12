#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../utils/util.h"
#include "queueFile.h"
#include "../sortedList/sortedList.h"

//int main()
//{
//    return 0;
//}

//int main()
//{
//    NodoQi_file *testaPtrF = NULL;
//    NodoQi_file *codaPtrF = NULL;
//
//    pushFile(&testaPtrF, &codaPtrF, "CANE", "GATTO");
//    pushFile(&testaPtrF, &codaPtrF, "CANE2", "GATTO2");
//
//    stampaQueueFile(testaPtrF);
//
//    char *string = NULL;
//    char *fileName = NULL;
//
//    popString(&testaPtrF, &codaPtrF, &string, &fileName);
//    printf("Stringa: %s\nFilename: %s\n", string, fileName);
//
//    free(string);
//    free(fileName);
//
//    freeQueueFile(&testaPtrF, &codaPtrF);
//    return 0;
//}


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

//void stampaQueueFile(NodoQiPtr_File lPtrF)
//{
//    if(lPtrF != NULL)
//    {
//        printf("%s | %s -> ", lPtrF->stringa, lPtrF->filename);
//
//        stampaQueueFile(lPtrF->prossimoPtr);
//    }
//    else
//        puts("NULL");
//}

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
