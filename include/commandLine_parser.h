#ifndef commandLine_parser_h
#define commandLine_parser_h

struct nodoCLP
{
    char option;
    char *argument;
    struct nodoCLP *prossimoPtr;
};
typedef struct nodoCLP NodoCLP;
typedef NodoCLP *NodoCLPtr;

void pushCoda(NodoCLPtr *testaPtrF, NodoCLPtr *codaPtrF, char optionF, char argumentF[]);
int popCoda(NodoCLPtr *lPtrF, NodoCLPtr *codaPtrF, char *optionF, char **argumentF);
int findOption(NodoCLPtr lPtr, char opt);
int existsDifferentOpt(NodoCLPtr lPtr);
int equalToLptr(NodoCLPtr lPtr, char opt);
int cmlParsing(NodoCLPtr *testaPtr, NodoCLPtr *codaPtr, int argc, char *argv[], char **sockName_f, bool *isPresent_p, long *time);
int topCoda(NodoCLPtr lPtrF, char *optionF, char **argumentF);
void stampaCoda(NodoCLPtr lPtrF);
void freeCoda(NodoCLPtr *lPtrF, NodoCLPtr *codaPtr);
long stringToLong(const char* s);

#endif