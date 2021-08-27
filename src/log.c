#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../utils/util.h"
#include "../include/log.h"

void writeLogFd_N_Date(FILE *logFile, int clientFd)
{
    assert(logFile != NULL);

    time_t timer;
    char timeString[20];
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(timeString, 20, "%d-%m-%Y %H:%M:%S", tm_info);

    DIE(fprintf(logFile, "Thread Worker: %ld \tClient fd: %d \tData: %s\n", pthread_self(), clientFd, timeString));
}

char *getCurrentTime()
{
    time_t timer;
    char *timeString = malloc(sizeof(char) * 20);
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(timeString, 20, "%d-%m-%Y %H:%M:%S", tm_info);

    return timeString;
}