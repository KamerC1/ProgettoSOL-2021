#ifndef soertedList_h
#define soertedList_h

struct nodoSL
{
    int dato;
    struct nodoSL *prossimoPtr;
};
typedef struct nodoSL NodoSL;
typedef NodoSL *NodoSLPtr;

//inserisce un elemento in una lista ordinata (in caso di errore, termina il processo)
void insertSortedList(NodoSLPtr *lPtr, int valore);
//ritorna -1 se l'elemento non viene trovato, 0 altrimenti)
int findSortedList(NodoSLPtr lPtr, int valore);
//(ritorna -1 in caso di errore, 0 altrimenti)
int deleteSortedList(NodoSLPtr *lPtr, int val);
void stampaSortedList(NodoSLPtr lPtr);
void freeSortedList(NodoSLPtr *lPtrF);

#endif