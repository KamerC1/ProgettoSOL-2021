#if !defined(CONN_H)
#define CONN_H

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_BYTES_SOCKNAME 256 
#define MAXBACKLOG 108

#define CONNESSIONE_RIFIUTATA 9
#define CONNESSIONE_ACCETTATA 10
#define API_OPENFILE 11
#define API_READFILE 12
#define API_READNFILES 13
#define API_WRITEFILE 14
#define API_APPENDTOFILE 15
#define API_CLOSEFILE 16
#define API_REMOVEFILE 17
#define API_LOCKFILE 18
#define API_UNLOCKFILE 19
#define API_REMOVE_CLIENT_INFO 20
#define API_ISFILE_PRESENT 21
#define API_COPY_FILE_TODIR 22
#define API_GET_FILE_SIZEBYTE 23




//Valore da inviare al server quando un argomento delle API è NULL
#define ARGUMENT_NULL 0 

#define WRITEN(fd, buf, size, text) if(writen(fd, buf, size)==-1) { PRINT(text); return -1;}
//#define READN(fd, buf, size, text) if(readn(fd, buf, size)==-1) { PRINT(text); return -1;}

//restituisce -1 se readn restituisce 0 o -1
//Le parentesi "{}" servono per distruggere le variabili locali
#define READN(fd, buf, size, text)                      \
{                                                       \
    int temp = readn(fd, buf, size);                    \
    if(temp == 0)                                       \
    {                                                   \
        PRINT("READN non ha letto nullo");              \
        return -1;                                      \
    }                                                   \
    else if(temp == -1)                                 \
    {                                                   \
        PRINT(text);                                    \
        return -1;                                      \
    }                                                   \
}                                                       \

/*
//restituisce -1 se rean restituisce 0 o -1
//Le parentesi "{}" servono per distruggere le variabili locali
#define WRITEN_C(fd, buf, size, text)                      \
    if(writen(fd, buf, size)==-1)                          \
    {                                                      \
        if(errno = ECONNRESET)                             \
            return -1;                                     \
    }                                                      \
    else    

*/

#define WRITEN_C(fd, buf, size, text)                                                           \
if(writen(fd, buf, size) == -1)                                                                 \
{                                                                                               \
    if(errno == ECONNRESET || errno == EPIPE)                                                   \
    {                                                                                           \
        PRINT("Impossibile fare la write: connessione chiusa dal client. Vado avanti")          \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        perror("API_WRITEFILE: writen()");                                                      \
        exit(EXIT_FAILURE);                                                                     \
    }                                                                                           \
}                                                                                               \

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}


#endif /* CONN_H */