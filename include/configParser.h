#ifndef configParser_h
#define configParser_h

#include <stdbool.h>


struct configFile_t {
    size_t maxStorageBytes;
    size_t maxStorageFiles;

    size_t numWorker;

    short fileReplacementAlg;

    char *socketName;
    char *logFile;
};
typedef struct configFile_t ConfigFile_t;

#define MAX_FILE "MAX_FILE"
#define MAX_BYTES "MAX_BYTES"
#define MAX_WORKER "MAX_WORKER"
#define SOCKNAME "SOCKNAME"
#define LOGFILE "LOGFILE"
#define REPLACEMENTE_ALG "REPLACEMENTE_ALG"

#define FIFO 0
#define LRU 1

#define DEFAULT_FILE 100
#define DEFAULT_BYTES 2048
#define DEFAULT_WORKER 10
#define DEFAULT_SOCKET "cs_sock"
#define DEFAULT_LOG "log.txt"
#define DEFAULT_REPLACEMENTE_ALG FIFO


ConfigFile_t *configServer(char stringFile[]);
int lineParsing(char line[], ConfigFile_t *configFile);
long isNumber(const char* s);
bool containsChar(char string[], char value);
void setDefaultConfigFile(ConfigFile_t *configFile);
void printConfig(ConfigFile_t configFile);
int findKey(char key[], char value[], ConfigFile_t *configFile);
void freeConfigFile(ConfigFile_t *configFile);

#endif