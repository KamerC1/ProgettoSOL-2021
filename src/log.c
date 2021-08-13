#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../utils/util.h"
#include "../include/log.h"

//void writeLog(FILE *logFile, int clientFd, char operazione[], char pathname[]);
//
//int main()
//{
//    //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
//    FILE *logFile = fopen(LOG_FILE, "a");
//    CS(logFile == NULL, "createReadNFiles: fopen()", errno)
//
//    writeLog(logFile, 1, "WRITE", "CANE:TXT");
//    fprintf(logFile, "Operazione: %s\tPathname %s\t", operazione, pathname);
//
//    writeLog(logFile, 2, "WRITE", "CANE:TXT");
//    fprintf(logFile, "Operazione: %s\tPathname %s\t", operazione, pathname);
//
//
//    SYSCALL_NOTZERO(fclose(logFile), "createReadNFiles: fclose() - termino processo")
//}

void writeLogFd_N_Date(FILE *logFile, int clientFd)
{
    assert(logFile != NULL);

    time_t timer;
    char timeString[20];
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(timeString, 20, "%d-%m-%Y %H:%M:%S", tm_info);

    fprintf(logFile, "Client fd: %d\tData: %s\n", clientFd, timeString);

}