struct nodo
{
    int dato;
    struct nodo *prossimoPtr;
};
typedef struct nodo Nodo;
typedef Nodo *NodoPtr;

//inserisce un elemento in una lista ordinata (in caso di errore, termina il processo)
void insertSortedList(NodoPtr *lPtr, int valore);
//ritorna -1 se l'elemento non viene trovato, 0 altrimenti)
int findSortedList(NodoPtr lPtr, int valore);
//(ritorna -1 in caso di errore, 0 altrimenti)
int deleteSortedList(NodoPtr *lPtr, int val);
void stampaSortedList(NodoPtr lPtr);
void freeSortedList(NodoPtr *lPtrF);
