#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include "util.h"

#define MAX_PATH_LENGTH 4097 //aggiunto il +1 per contenere '\0'
#define MAX_FILE_LENGTH 256 //aggiunto il +1 per contenere '\0'

//Ritorna il path assoluto di pathname. Null in caso di errore
char *getRealPath(const char pathname[])
{
    char *absolutePath = realpath(pathname, NULL);
    char *temp = realloc(absolutePath, sizeof(char) * (strlen(absolutePath) + 1));

    if(temp == NULL)
    {
        PRINT("getRealPath: realloc");
        free(absolutePath);
        return NULL;
    }

    return temp;
}

//Controlla se pathname è corretto
int checkPathname(const char *pathname)
{
    if(pathname == NULL)
        return -1;

    //basename può modificare pathname
    int lengthPath = strlen(pathname);
    char pathnameCopy[lengthPath+1];
    strncpy(pathnameCopy, pathname, lengthPath+1);

    if(lengthPath >= MAX_PATH_LENGTH || basename(pathnameCopy) == NULL || strlen(pathnameCopy) >= MAX_FILE_LENGTH)
        return -1;

    return 0;
}