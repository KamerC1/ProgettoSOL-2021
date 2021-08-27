#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include "../include/configParser.h"
#include "../utils/flagsReplacementAlg.h"

#include "../utils/util.h"

//ritorna NULL in caso di errore, altrimenti la struttura che contiene i dati sulla configurazione del server
//stringFile: pathname del file da leggere
ConfigFile_t *configServer(char stringFile[])
{
    ConfigFile_t *configStruct = malloc(sizeof(ConfigFile_t));
    NULL_SYSCALL(configStruct, "configParser: malloc")
    setDefaultConfigFile(configStruct);
    FILE *configFile = fopen(stringFile, "r");

    if(configFile == NULL)
    {
        puts("Configurazione fallita, imposto valori di default");
        return configStruct;
    }

    char *fileLine = NULL;
    size_t n = 0;
    size_t lineLength = 0;
    errno = 0;
    bool printMsg = false;
    while ((lineLength = getline(&fileLine, &n, configFile) != -1))
    {
        if(errno != 0)
        {
            puts("Configurazione fallita, imposto valori di default");
            if(fileLine != NULL)
                free(fileLine);
            return configStruct;
        }

        //La riga contiene sicurem
        if(isalpha(fileLine[0]) == 0)
        {
            errno = 0;
            n = 0;
            free(fileLine);
            continue;
        }

        if(lineParsing(fileLine, configStruct) == -1)
        {
            printMsg = true;
            if(fileLine != NULL)
                free(fileLine);
            n = 0;
            continue;
        }


        n = 0;
        free(fileLine);
        errno = 0;
    }

    if(printMsg == true)
        puts("Configurazione fallita, imposto valori di default dove necessario"); //Nota: i valori letti correttamente, rimangono memorizzati

    if(fileLine)
        free(fileLine);

    SYSCALL_NOTZERO(fclose(configFile), "configServer: fclose() - termino processo")

    return configStruct;
}

//Imposta i valori di default in configFile
void setDefaultConfigFile(ConfigFile_t *configFile)
{
    configFile->maxStorageFiles = DEFAULT_FILE;
    configFile->maxStorageBytes = DEFAULT_BYTES;

    configFile->numWorker = DEFAULT_WORKER;
    configFile->fileReplacementAlg = DEFAULT_REPLACEMENTE_ALG;


    size_t lenLog = strlen(DEFAULT_LOG);
    size_t lenSocketName = strlen(DEFAULT_SOCKET);

    configFile->logFile = malloc(sizeof(char) * (lenLog + 1));
    NULL_SYSCALL(configFile->logFile, "malloc")
    memset(configFile->logFile, '\0', lenLog + 1);
    configFile->socketName = malloc(sizeof(char) * (lenSocketName + 1));
    NULL_SYSCALL(configFile->socketName, "malloc")
    memset(configFile->logFile, '\0', lenSocketName + 1);


    strncpy(configFile->logFile, DEFAULT_LOG, strlen(DEFAULT_LOG)+1);
    strncpy(configFile->socketName, DEFAULT_SOCKET, strlen(DEFAULT_SOCKET)+1);
}

//libera la memoria di "configFile" e dealloc lo stesso "configFile"
void freeConfigFile(ConfigFile_t *configFile)
{
    assert(configFile->logFile != NULL);
    assert(configFile->socketName != NULL); //a questo punto dell'esecuzione, non possono essere != NULL (grazie ai valori di default)

    free(configFile->logFile);
    free(configFile->socketName);

    free(configFile);

}

//Suddivida la stringa in key=value e vi chiama findKey
//ritorna -1 in caso di errore, 0 altrimenti
int lineParsing(char line[], ConfigFile_t *configFile)
{
    char *savePtr;
    char *keyToken = strtok_r(line, "=", &savePtr);
    if(keyToken == NULL)
    {
        return -1;
    }

    char *valueToken = strtok_r(NULL, ";", &savePtr);
    if(valueToken == NULL)
    {
        return -1;
    }


    return findKey(keyToken, valueToken, configFile);
}

//imposta il valore value in configFile in base alla stringa "key"
//ritorna -1 in caso di errore.
int findKey(char key[], char value[], ConfigFile_t *configFile)
{
    if(strcmp(key, MAX_FILE) == 0)
    {
        long valueLong = isNumber(value);
        CS(valueLong <= 0, "Errore: isNumber", errno);

        configFile->maxStorageFiles = valueLong;
    }
    else if(strcmp(key, MAX_BYTES) == 0)
    {
        long valueLong = isNumber(value);
        CS(valueLong <= 0, "Errore: isNumber", errno);

        configFile->maxStorageBytes = valueLong;
    }
    else if(strcmp(key, MAX_WORKER) == 0)
    {
        long valueLong = isNumber(value);
        CS(valueLong <= 0, "Errore: isNumber", errno);

        configFile->numWorker = valueLong;
    }
    else if(strcmp(key, LOGFILE) == 0)
    {
        if(containsChar(value, ' ') || containsChar(value, '\t') || containsChar(value, '\n') || containsChar(value, '\f'))
        {
            PRINT("Stringa mal formattata")
            return -1;
        }

        free(configFile->logFile);

        size_t keyLength = strlen(value);
        configFile->logFile = malloc(sizeof(char) * (keyLength+1));
        NULL_SYSCALL(configFile->logFile, "malloc")
        memset(configFile->logFile, '\0', keyLength+1);
        strncpy(configFile->logFile, value, keyLength);
    }
    else if(strcmp(key, SOCKNAME) == 0)
    {
        if(containsChar(value, ' ') || containsChar(value, '\t') || containsChar(value, '\n') || containsChar(value, '\f'))
        {
            PRINT("Stringa mal formattata")
            return -1;
        }

        free(configFile->socketName);

        size_t keyLength = strlen(value);
        configFile->socketName = malloc(sizeof(char) * (keyLength+1));
        NULL_SYSCALL(configFile->logFile, "malloc")
        memset(configFile->socketName, '\0', keyLength+1);
        strncpy(configFile->socketName, value, keyLength);
    }
    else if(strcmp(key, REPLACEMENTE_ALG) == 0)
    {
        if(strcmp(value, "FIFO") == 0)
        {
            configFile->fileReplacementAlg = FIFO;
        }
        else if(strcmp(value, "LRU") == 0)
        {
            configFile->fileReplacementAlg = LRU;
        }
        else if(strcmp(value, "MRU") == 0)
        {
            configFile->fileReplacementAlg = MRU;
        }
        else if(strcmp(value, "LFU") == 0)
        {
            configFile->fileReplacementAlg = LFU;
        }
        else if(strcmp(value, "MFU") == 0)
        {
            configFile->fileReplacementAlg = MFU;
        }
    }

    return 0;
}

//stampa i valori della struttura
void printConfig(ConfigFile_t configFile)
{
    printf("MAX_WORKER: %ld\n", configFile.numWorker);
    printf("SOCKNAME: %s\n", configFile.socketName);
    printf("MAX_BYTES: %ld\n", configFile.maxStorageBytes);
    printf("MAX_FILE: %ld\n", configFile.maxStorageFiles);
    printf("LOGFILE: %s\n", configFile.logFile);
    printf("REPLACEMENTE_ALG: %d\n", configFile.fileReplacementAlg);
}

long isNumber(const char* s)
{
    char* e = NULL;
    errno = 0;
    long val = strtol(s, &e, 0);
    if (e != NULL && *e == (char)0)
    {
        if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE)
        {
            perror("Numero troppo grande o piccolo: ");
            exit(EXIT_FAILURE);
        }

        return val;
    }
    return -1;
}

//ritoran true se viene trovato "value" in "string"
bool containsChar(char string[], char value)
{
    const char *posPtr = strchr(string, value);
    if(posPtr != NULL)
    {
        return true;
    }

    return false;
}