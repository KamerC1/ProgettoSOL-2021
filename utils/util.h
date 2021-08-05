#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define READ_END 0
#define WRITE_END 1


#define RETURN_SYSCALL(r,c,e) if((r=c)==-1) { perror(e);exit(errno); }
#define SYSCALL(c,e) if(c==-1) { perror(e);exit(errno);}
//usare con le funzioni che ritornano NULL quando falliscono e di cui si vuole memorizzare il valore di ritorno (es: fopen)
#define RETURN_NULL_SYSCALL(retrunVar, fun, text) if((retrunVar=fun) == NULL) { perror(text);exit(errno); }
//usare per le syscall che quando falliscono ritornano un valore != 0
#define SYSCALL_NOTZERO(syscall, text) if(syscall != 0) {perror(text);exit(errno);}
#define THREAD_CREATE(a, b, c, d, text) if(pthread_create(a, b, c, d) != 0) { perror(text);exit(EXIT_FAILURE);}
#define THREAD_JOIN(a, b, text) if(pthread_join(a, b) != 0) { perror(text);exit(EXIT_FAILURE);}

#define STAMPA_ERRORE 1 
#define PRINT(text) if(STAMPA_ERRORE) {puts(text);}

#define CS(cond, text, err) if(cond) {PRINT(text);errno=err; return -1;} //CS = COND_SETERR
//uguale a "COND_SETERR" ma ritorna NULL
#define CSN(cond, text, err) if(cond) {PRINT(text);errno=err; return NULL;} //CS = COND_SETERR_NULL

//esegue delle istruzioni "act" in caso di errore
#define CSA(cond, text, err, act) if(cond) {PRINT(text); act; errno=err; return -1;} //CSA = COND_SETERR_ACT
#define CSAN(cond, text, err, act) if(cond) {PRINT(text); act; errno=err; return NULL;} //CSA = COND_SETERR_ACT_NULL

#define FCLOSE(file) if(fclose(file)==EOF) {exit(errno);}
#define CLOSEDIR(dir) if(closedir(dir)==EOF) {exit(errno);}








#define LOCK(l)                                         \
if (pthread_mutex_lock(l) != 0)                         \
{	                                                    \
    fprintf(stderr, "ERRORE FATALE lock\n");			\
    pthread_exit((void*)EXIT_FAILURE);                  \
}

#define UNLOCK(l)                                       \
if (pthread_mutex_unlock(l) != 0)                       \
{	                                                    \
    fprintf(stderr, "ERRORE FATALE unlock\n");			\
    pthread_exit((void*)EXIT_FAILURE);                  \
}

#define SIGNAL(c)                                       \
if (pthread_cond_signal(c) != 0)                        \
{	                                                    \
    fprintf(stderr, "ERRORE FATALE signal\n");			\
    pthread_exit((void*)EXIT_FAILURE);                  \
}

#define WAIT(c, l)                                      \
if (pthread_cond_wait(c,l) != 0)                        \
{	                                                    \
    fprintf(stderr, "ERRORE FATALE wait\n");			\
    pthread_exit((void*)EXIT_FAILURE);                  \
}

#define SCANF_STRINGA(stringa)                \
if(scanf("%s", stringa) == 0)                 \
{                                             \
    perror("Impossibile leggere la stringa"); \
    exit(EXIT_FAILURE);                       \
}

#define REALLOC(ptr, size) \
char *ptr1 = realloc(ptr, size); \
if (ptr1 == NULL)          \
{                          \
    puts("REALLOC error"); \
    free(ptr);             \
    exit(EXIT_FAILURE);    \
}                          \
else                       \
    ptr = ptr1;             \

