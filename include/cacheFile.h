#ifndef cacheFile_h
#define cacheFile_h

#include "../strutture_dati/queueFile/queueFile.h"
#include "../include/serverAPI.h"

struct queueFile 
{
    NodoQi_file *testaFilePtr;
    NodoQi_file *codaFilePtr;
};
typedef struct queueFile QueueFile_t;


ServerStorage *createStorage(size_t maxStorageBytesF, size_t maxStorageFilesF, short fileReplacementAlgF);
void addFile2Storage(File *serverFile, ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF);
void FIFO_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, char *pathnameCaller);
void stampaStorage(ServerStorage *storage);
void addBytes2Storage(File *serverFile, ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF);
int copyFile2Dir(NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, const char *dirname);
void LRU_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *callerFile);
void LFU_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *callerFile);
void MFU_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *callerFile);
void MRU_ReplacementAlg(ServerStorage *storage, NodoQiPtr_File *testaPtrF, NodoQiPtr_File *codaPtrF, File *callerFile);
#endif