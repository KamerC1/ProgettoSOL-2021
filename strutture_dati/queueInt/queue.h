struct nodoQi
{
    int data;
    struct nodoQi *prossimoPtr;
};
typedef struct nodoQi NodoQi;
typedef NodoQi *NodoQiPtr;


void push(NodoQiPtr *testaPtrF, NodoQiPtr *codaPtrF, int dataF);
int pop(NodoQiPtr *lPtrF);
int top(NodoQiPtr lPtrF);
void stampa(NodoQiPtr lPtrF);