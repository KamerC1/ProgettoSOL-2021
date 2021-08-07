#ifndef commandLine_parser_h
#define commandLine_parser_h

#endif

struct nodoCLP
{
    char option;
    char *argument;
    struct nodoCLP *prossimoPtr;
};
typedef struct nodoCLP NodoCLP;
typedef NodoCLP *NodoCLPtr;

void pushCoda(NodoCLPtr *testaPtrF, NodoCLPtr *codaPtrF, char optionF, char argumentF[]);
int popCoda(NodoCLPtr *lPtrF, char *optionF, char **argumentF);
int findOption(NodoCLPtr lPtr, char opt);
int equalTolastOpt(NodoCLPtr codaPtr, char opt);
void cmlParsing(NodoCLPtr *testaPtr, NodoCLPtr *codaPtr, int argc, char *argv[]);
int topCoda(NodoCLPtr lPtrF, char *optionF, char **argumentF);
void stampaCoda(NodoCLPtr lPtrF);
void freeLista(NodoCLPtr *lPtrF, NodoCLPtr *codaPtr);
