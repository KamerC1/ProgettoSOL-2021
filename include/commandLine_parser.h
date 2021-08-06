#ifndef commandLine_parser_h
#define commandLine_parser_h

#endif

struct nodo
{
    char option;
    char *argument;
    struct nodo *prossimoPtr;
};
typedef struct nodo Nodo;
typedef Nodo *NodoPtr;

void pushCoda(NodoPtr *testaPtrF, NodoPtr *codaPtrF, char optionF, char argumentF[]);
int popCoda(NodoPtr *lPtrF, char *optionF, char **argumentF);
int findOption(NodoPtr lPtr, char opt);
int equalTolastOpt(NodoPtr codaPtr, char opt);
void cmlParsing(NodoPtr *testaPtr, NodoPtr *codaPtr, int argc, char *argv[]);
int topCoda(NodoPtr lPtrF, char *optionF, char **argumentF);
void stampaCoda(NodoPtr lPtrF);
void freeLista(NodoPtr *lPtrF, NodoPtr *codaPtr);
