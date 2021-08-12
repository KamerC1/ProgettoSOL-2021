#ifndef queueChar_h
#define queueChar_h

struct nodoQi_string
{
    char *stringa;
    struct nodoQi_string *prossimoPtr;
};
typedef struct nodoQi_string NodoQi_string;
typedef NodoQi_string *NodoQiPtr_string;


void pushStringa(NodoQiPtr_string *testaPtrF, NodoQiPtr_string *codaPtrF, char dataF[]);
char *popString(NodoQiPtr_string *lPtrF, NodoQiPtr_string *codaPtr);
//int top(NodoQiPtr_string lPtrF);
//int findDataQueue(NodoQiPtr_string lPtr, int data);
int deleteDataQueueStringa(NodoQiPtr_string *lPtr, NodoQiPtr_string *codaPtr, char *data);
void stampaQueueStringa(NodoQiPtr_string lPtrF);
void freeQueueStringa(NodoQiPtr_string *lPtrF, NodoQiPtr_string *codaPtr);


#endif