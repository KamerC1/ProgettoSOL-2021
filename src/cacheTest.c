#include <assert.h>

#include "../utils/util.h"
#include "serverAPI.c"

//RIMUOVERE [controllare]

#define MAX_STORAGE_BYTES 10
#define MAX_STORAGE_FILES 1
#define FIFO 0

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        puts("Argomenti mancanti");
    }

    ServerStorage *storage = createStorage(MAX_STORAGE_BYTES, MAX_STORAGE_FILES, FIFO);

    pushStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), "CANE");
    pushStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), "GATTO");
    stampaQueueStringa(storage->FIFOtestaPtr);

    deleteDataQueueStringa(&(storage->FIFOtestaPtr), &(storage->FIFOcodaPtr), "GATTO");
    stampaQueueStringa(storage->FIFOtestaPtr);

    //de-allocazione memoria
    SYSCALL(icl_hash_destroy(storage->fileSystem, free, freeFileData), "ERRORE")
    free(storage);

    return 0;
}
