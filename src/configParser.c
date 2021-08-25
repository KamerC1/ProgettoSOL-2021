#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include "../include/configParser.h"

#include "../utils/util.h"

//int main(int argc, char *argv[])
//{
//    if(argc <= 1)
//    {
//        puts("Arg mancante");
//        exit(EXIT_FAILURE);
//    }
//
//    ConfigFile_t *configStruct = malloc(sizeof(ConfigFile_t));
//    NULL_SYSCALL(configStruct, "configParser: malloc")
//    setDefaultConfigFile(configStruct);
//
//    //il file, anche se esiste già, viene sovrascritto perché il file nel server potrebbe essere cambiato
//    FILE *configFile = fopen(argv[1], "r");
//    CS(configFile == NULL, "creatFileAndCopy: fopen()", errno)
//
//
//
//    char *fileLine = NULL;
//    size_t n = 0;
//    size_t lineLength = 0;
//    errno = 0;
//    while ((lineLength = getline(&fileLine, &n, configFile) != -1))
//    {
//        CSA(errno != 0, "getline", errno, if(fileLine != NULL) free(fileLine))
//
//        if(isalpha(fileLine[0]) == 0)
//        {
//            n = 0;
//            free(fileLine);
//            continue;
//        }
//
//        printf("%s\n", fileLine);
//        lineParsing(fileLine, configStruct);
////        printf("%ld\n", lineLength);
//
//
//        n = 0;
//        free(fileLine);
//        errno = 0;
//    }
//
//    if(fileLine)
//        free(fileLine);
//
//    puts("====STAMPA====");
//    printConfig(*configStruct);
//
//    freeConfigFile(configStruct);
//
//    SYSCALL_NOTZERO(fclose(configFile), "creatFileAndCopy: fclose() - termino processo")
//
//
//    return 0;
//}

//ritorna NULL in caso di errore, altrimenti la struttura che contiene i dati sulla configurazione del server
//stringFile è il file dove vi è la configurazione
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





























////ritorna la posizione della prima occorrenza del delimitatore "delim" in "string"
////ritorna -1 se non è stato trovato
//int getFirstOccurence(char string[], char delim)
//{
//    const char *posPtr = strchr(string, delim);
//    if(posPtr != NULL)
//    {
//        int pos = posPtr - string;
//        string[pos] = '\0';
//        printf("Pos: %d\n", pos);
//        printf("Strlen: %ld\n", strlen(string));
//
//        return pos;
//    }
//
//    return -1;
//}
























//void deleteSpace(char string[])
//{
//    size_t stringLen = strlen(string);
//    char noSpaceString[stringLen+1];
//
//    for(int i = 0; i < stringLen; i++)
//    {
//        if(isNumber(string[i]) == )
//    }
//}

//const char *posPtr = strchr(valueToken, '\n');
//if(posPtr != NULL)
//{
//int pos = posPtr - valueToken;
//valueToken[pos] = '\0';
//printf("Pos: %d\n", pos);
//printf("Strlen: %ld\n", strlen(valueToken));
//}

//int lineParsing(char line[])
//{
//    puts("Nuovo giro");
//
//    unsigned short i = 0;
//    char *savePtr;
//    char *keyToken = strtok_r(line, "=", &savePtr);
//
//    if(keyToken == NULL)
//    {
//        return -1;
//    }
//
//    char *valueToken = strtok_r(NULL, ";", &savePtr);
//    if(valueToken == NULL)
//    {
//        return -1;
//    }
//
//    //elimina tab o spazio da keyToken
//    int spacePosition = getFirstOccurence(keyToken, ' ');
//    int tabPosition = getFirstOccurence(keyToken, '\t');
//
//    if(spacePosition >= 0)
//        keyToken[spacePosition] = '\0';
//    else if(tabPosition >= 0)
//        keyToken[tabPosition] = '\0';
//
//    printf("keyToken:-%s-\n", keyToken);
//
//
//    //elimina tab o spazio da valueToken
//    spacePosition = getFirstOccurence(valueToken, ' ');
//    tabPosition = getFirstOccurence(valueToken, '\t');
//
//    if(spacePosition >= 0)
//    {
//        puts("ciao space");
//        valueToken[spacePosition] = '\0';
//    }
//    if(tabPosition >= 0)
//    {
//        puts("ciao tab");
//        valueToken[tabPosition] = '\0';
//    }
//
//
//    printf("valueToken:-%s-\n", valueToken);
//
////    if(strcmp(token, "MAX_FILE") == 0)
////    {
////        long value = isNumber(token)
////    }
//
//    return 0;
//}


//printf("valueToken:-%s-\n", valueToken);
//
//if(strcmp(keyToken, "MAX_FILE") == 0)
//{
//long value = isNumber(valueToken);
//CS(value == -1, "isNumber", errno);
//printf("KEI INT: %ld\n", value);
//}