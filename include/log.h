#ifndef log_h
#define log_h

#define LOG_FILE "log.txt"

//stampa sul log l'esito dell'operazione - val == 1 ==> Positovo 
#define ESOP(text, val)                                                             \
    if(val == 1)                                                                    \
    {                                                                               \
        if(fprintf(storage->logFile, "Esito %s: POSITIVO\n\n", text) == -1)         \
            exit(errno);                                                            \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        if(fprintf(storage->logFile, "Esito %s: NEGATIVO\n\n", text) == -1)         \
            exit(errno);                                                            \
    }                                                                              \



void writeLogFd_N_Date(FILE *logFile, int clientFd);
char *getCurrentTime();

#endif