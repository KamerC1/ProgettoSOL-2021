#ifndef RWLOCK_h
#define RWLOCK_h

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "../../utils/util.h"
#include "../hash/icl_hash.h"


struct rwLockStruct{
    pthread_mutex_t mutexFile;  //g
    pthread_cond_t condFile;    //cond
    unsigned int num_readers_active;
    unsigned int num_writers_waiting;
    bool writer_active;
};
typedef struct rwLockStruct RwLock_t;

void rwLock_init(RwLock_t *rwLockF);
void rwLock_startReading(RwLock_t *rwLockF);
void rwLock_endReading(RwLock_t *rwLockF);
void rwLock_startWriting(RwLock_t *rwLockF);
int rwLock_startWriting_p(RwLock_t *rwLockF, icl_hash_t *hashPtrF, const char *pathname);
void rwLock_endWriting(RwLock_t *rwLockF);

#endif