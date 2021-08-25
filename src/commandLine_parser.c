#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include "../utils/util.h"
#include "../include/commandLine_parser.h"

//ritorna 0 in caso di successo, altrimenti -1.
//*sockName_f: memorizza il socket presente in -f (se presente)
//isPresent_p è true se è stata passata l'opzione -p
int cmlParsing(NodoCLPtr *testaPtr, NodoCLPtr *codaPtr, int argc, char *argv[], char **sockName_f, bool *isPresent_p, long *time)
{
    //memorizza il socket presente in -f

    //controllano se l'opzione: h,f,p sono presenti più volte.
    bool isPresent_h = false;
    bool isPresent_f = false;
    bool isPresent_t = false;
    *isPresent_p = false;

    //controlla se ci sono presenti opzioni che richiedono -f
    bool need_f = false;

    *time = 0; //se -t non è specificato, *time è 0 di default

    int opt;
    while ((opt = getopt(argc, argv, ":hf:w:W:D:r:R::d:l:u:c:pt:a:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                if(isPresent_h == false)
                {
                    pushCoda(testaPtr, codaPtr, opt, NULL);
                    isPresent_h = true;
                }
                else PRINT("-h già prsente - ignorato")
                break;
            case 'f':
                if(isPresent_f == false)
                {
//                    pushCoda(testaPtr, codaPtr, opt, optarg);
                    isPresent_f = true;

                    size_t optArgLen = strlen(optarg);
                    *sockName_f = malloc(sizeof(char) * (optArgLen + 1));
                    NULL_SYSCALL(*sockName_f, "cmlParsing: malloc")
                    memset(*sockName_f, '\0', optArgLen + 1);
                    strncpy(*sockName_f, optarg, optArgLen);
                }
                else PRINT("-f già prsente - ignorato")
                break;
            case 'w':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'W':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'a':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'D':
                if(equalToLptr(*codaPtr, 'W') == 1 || equalToLptr(*codaPtr, 'w') == 1 || equalToLptr(*codaPtr, 'a') == 1)
                {
                    need_f = true;
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                }
                else
                    PRINT("-D non segue -W o -w");

                break;
            case 'r':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'R':
                need_f = true;
                if(optarg == NULL)
                    pushCoda(testaPtr, codaPtr, opt, "0");
                else
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'd':
                if(equalToLptr(*codaPtr, 'r') || equalToLptr(*codaPtr, 'R'))
                {
                    need_f = true;
                    pushCoda(testaPtr, codaPtr, opt, optarg);
                }
                else
                    puts("-d non segue -r o -R");
                break;
            case 't':
                if(isPresent_t == false)
                {
                    need_f = true;
                    *time = stringToLong(optarg);
                    if(*time < 0) //in caso di errore, o se il numero è negativo, viene impostato a 0
                        *time = 0;
                    isPresent_t = true;
                }
                else PRINT("-p già presente - ignorato")
                break;
            case 'l':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'u':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'c':
                need_f = true;
                pushCoda(testaPtr, codaPtr, opt, optarg);
                break;
            case 'p':
                if(*isPresent_p == false)
                {
                    need_f = true;
//                    pushCoda(testaPtr, codaPtr, opt, optarg); //Non c'è bisogno di inserirlo nella coda
                    *isPresent_p = true;
                }
                else PRINT("-p già presente - ignorato")
                break;
            case ':': {
                printf("l'opzione '-%c' richiede un argomento\n", optopt);
            }
                break;
            case '?': {  // restituito se getopt trova una opzione non riconosciuta
                printf("l'opzione '-%c' non e' gestita\n", optopt);
            }
                break;
            default:;
        }
    }
    //Può capitare che la lista sia vuota perché nessun argomento è stato accettato
    if(*testaPtr == NULL && *isPresent_p == false && isPresent_f == false)
    {
        puts("Argomenti non accettati");

        errno = EINVAL;
        return -1;
    }

    //è stato inserito un comando che richiede l'opzione -f
    if(isPresent_f == false && need_f == true)
    {
        PRINT("Connessione non stabilita [opzione -f mancante]")
        freeCoda(testaPtr, codaPtr);
        free(*sockName_f);

        errno = EINVAL;
        return -1;
    }


    return 0;
}

void pushCoda(NodoCLPtr *testaPtrF, NodoCLPtr *codaPtrF, char optionF, char argumentF[])
{
    NodoCLPtr nuovoPtr = NULL;
    RETURN_NULL_SYSCALL(nuovoPtr, malloc(sizeof(NodoCLP)), "push: malloc(sizeof(Nodo))")


    nuovoPtr->option = optionF;

    //memorizza argumentF
    if(argumentF != NULL)
    {
        int argumentFLength = strlen(argumentF);
        RETURN_NULL_SYSCALL(nuovoPtr->argument, malloc(argumentFLength+1), "push: malloc(argumentFLength+1)")
        strncpy(nuovoPtr->argument, argumentF, argumentFLength+1);
    } else
    {
        nuovoPtr->argument = NULL;
    }

    nuovoPtr->prossimoPtr = NULL;

    if(*testaPtrF == NULL)
    {
        *testaPtrF = nuovoPtr;
        *codaPtrF = nuovoPtr;
    }
    else
    {
        (*codaPtrF)->prossimoPtr = nuovoPtr;
        *codaPtrF = nuovoPtr;
    }
}

//argumentF deve essere NULL, altrimenti restituisce -1, errno = EPERM
int popCoda(NodoCLPtr *lPtrF, NodoCLPtr *codaPtrF, char *optionF, char **argumentF)
{
    CS(*lPtrF == NULL, "popCoda: la lista è vuota", EPERM)
    CS(*argumentF != NULL, "popCoda: argumentF NON è NULL", EPERM)

    *optionF = (*lPtrF)->option;

    if((*lPtrF)->argument != NULL)
    {
        *argumentF = (*lPtrF)->argument;
    }

    NodoCLPtr tempPtr = *lPtrF;
    *lPtrF = (*lPtrF)->prossimoPtr;
    if(*lPtrF == NULL)
        *codaPtrF = NULL;

    free(tempPtr);

    return 0;
}

//ritorna 1 se trova opt, 0 altrimenti
int findOption(NodoCLPtr lPtr, char opt)
{
    if(lPtr == NULL)
    {
        return 0;
    }
    if(lPtr->option == opt)
    {
        return 1;
    }
    else
        return findOption(lPtr->prossimoPtr, opt);
}

//restituisce 1 se l'opt di lPtr è uguale a opt
int equalToLptr(NodoCLPtr lPtr, char opt)
{
    if(lPtr == NULL)
    {
//        PRINT("\t\ttestaPtr: coda vuota")
        return 0;
    }


    if(lPtr->option == opt) {
        return 1;
    }


    return 0;
}

//argumentF deve essere NULL, altrimenti restituisce -1, errno = EPERM
int topCoda(NodoCLPtr lPtrF, char *optionF, char **argumentF)
{
    CS(lPtrF == NULL, "topCoda: la lista è vuota", EINTR)
    CS(*argumentF != NULL, "topCoda: argumentF NON è NULL", EPERM)

    *optionF = lPtrF->option;

    //copio argument in argumentF
    int argumentLength = strlen(lPtrF->argument);
    RETURN_NULL_SYSCALL(*argumentF, malloc(argumentLength + 1), "topCoda: malloc")
    strncpy(*argumentF, lPtrF->argument, argumentLength + 1);

    return 0;
}

void stampaCoda(NodoCLPtr lPtrF)
{
    if(lPtrF != NULL)
    {
        printf("[option: %c argument: %s]\n", lPtrF->option, lPtrF->argument);

        stampaCoda(lPtrF->prossimoPtr);
    }
    else
        puts("NULL");
}

void freeCoda(NodoCLPtr *lPtrF, NodoCLPtr *codaPtr)
{
    if(*lPtrF != NULL)
    {
        NodoCLPtr temPtr = *lPtrF;
        *lPtrF = (*lPtrF)->prossimoPtr;

        free(temPtr->argument);
        free(temPtr);

        freeCoda(lPtrF, codaPtr);
    }
    else
    {
        *codaPtr = NULL;
    }
}

//converte una stringa in un long
long stringToLong(const char* s)
{
    char* e = NULL;
    errno = 0;
    long val = strtol(s, &e, 0);
    if (e != NULL && *e == (char)0)
    {
        if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE)
        {
            perror("Numero troppo grande o piccolo: ");
            return -1;
        }

        return val;
    }
    return -1;
}