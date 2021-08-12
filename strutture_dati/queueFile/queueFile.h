#ifndef queueFile_h
#define queueFile_h

#include "../../include/serverAPI.h"

struct nodoQi_file
{
    File *file;
    struct nodoQi_file *prossimoPtr;
};
typedef struct nodoQi_file NodoQi_file;
typedef NodoQi_file *NodoQiPtr_File;


void pushFile(NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *fileF);
File *popFile(NodoQiPtr_File *lPtrF, NodoQiPtr_File *codaPtr);
//int top(NodoQiPtr_string lPtrF);
//int findDataQueue(NodoQiPtr_string lPtr, int data);
void freeQueueFile(NodoQiPtr_File *lPtrF, NodoQiPtr_File *codaPtr);
void stampaQueueFile(NodoQiPtr_File lPtrF);

#endif