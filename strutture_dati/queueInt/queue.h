#ifndef queueInt_h
#define queueInt_h

struct nodoQi
{
    int data;
    struct nodoQi *prossimoPtr;
};
typedef struct nodoQi NodoQi;
typedef NodoQi *NodoQiPtr;


void push(NodoQiPtr *testaPtrF, NodoQiPtr *codaPtrF, int dataF);
int pop(NodoQiPtr *lPtrF, NodoQiPtr *codaPtr);
int top(NodoQiPtr lPtrF);
int findDataQueue(NodoQiPtr lPtr, int data);
int deleteDataQueue(NodoQiPtr *lPtr, NodoQiPtr *codaPtr, int data);
void freeLista(NodoQiPtr *lPtrF, NodoQiPtr *codaPtr);
void stampa(NodoQiPtr lPtrF);

#endif