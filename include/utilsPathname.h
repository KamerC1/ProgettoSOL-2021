#ifndef utilsPathname_h
#define utilsPathname_h

#include <stdlib.h>

#define MAX_FILE_LENGTH 256 //aggiunto il +1 per contenere '\0'

char *getRealPath(const char pathname[]);
int checkPathname(const char *pathname);

#endif