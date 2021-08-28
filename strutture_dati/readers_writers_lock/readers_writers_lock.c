#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "../../utils/util.h"
#include "readers_writers_lock.h"

//Inizializza struttura dati
void rwLock_init(RwLock_t *rwLockF)
{
    SYSCALL_NOTZERO(pthread_mutex_init(&(rwLockF->mutexFile), NULL), "rwLock_init: pthread_mutex_init")
    SYSCALL_NOTZERO(pthread_cond_init(&(rwLockF->condFile), NULL), "rwLock_init: pthread_cond_init")

    rwLockF->num_readers_active = 0;
    rwLockF->num_writers_waiting = 0;
    rwLockF->writer_active = false;
}

void rwLock_startReading(RwLock_t *rwLockF)
{
//    LOCK(&(rwLockF->mutexFile))

    while(rwLockF->num_writers_waiting > 0 || rwLockF->writer_active == true)
    {
        WAIT(&(rwLockF->condFile), &(rwLockF->mutexFile))
    }
    rwLockF->num_readers_active++;

//    UNLOCK(&(rwLockF->mutexFile))
}

void rwLock_endReading(RwLock_t *rwLockF)
{
//    LOCK(&(rwLockF->mutexFile))

    rwLockF->num_readers_active--;
    if(rwLockF->num_readers_active == 0)
    {
        BCAST(&(rwLockF->condFile))
    }

//    UNLOCK(&(rwLockF->mutexFile))
}

void rwLock_startWriting(RwLock_t *rwLockF)
{
//    LOCK(&(rwLockF->mutexFile))

    rwLockF->num_writers_waiting++;
    while(rwLockF->num_readers_active > 0 || rwLockF->writer_active == true)
    {
        puts("\n\n\n\n\n\nAPSETOOOO\n\n\n\n\n\n\n");
        WAIT(&(rwLockF->condFile), &(rwLockF->mutexFile))
    }
    rwLockF->num_writers_waiting--;
    rwLockF->writer_active = true;

//    UNLOCK(&(rwLockF->mutexFile))
}

void rwLock_endWriting(RwLock_t *rwLockF)
{
//    LOCK(&(rwLockF->mutexFile))

    rwLockF->writer_active = false;
    BCAST(&(rwLockF->condFile))

//    UNLOCK(&(rwLockF->mutexFile))
}