//testa le api tra client e server
#include <assert.h>


#include "clientAPI.c"
#include "../strutture_dati/queueInt/queue.c"

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        puts("Inseririe argomenti");
        return -1;
    }
//    srand(time(NULL));
//    puts("Ciao");
//    struct timespec abstime;
//    abstime.tv_sec = time(NULL) + 10;
//    SYSCALL(openConnection("cs_sock", 1000, abstime), "Errore")
//
//        char *readBuf = NULL;
//        size_t readSize;
//
//    SYSCALL(openFile("Cane", O_CREATE), "Errore")
//    SYSCALL(readFile("Cane", &readBuf, &readSize), "Errore")
//
//    if(readBuf != NULL)
//    {
//        printf("readBuf: %s\n", readBuf);
//        free(readBuf);
//    }

//    SYSCALL(openFile("Gatto", O_CREATE), "Errore")
//    SYSCALL(appendToFile("Cane", "1234566790123", strlen("1234566790123") + 1, "../test/readN"), "ERRORE")
//    SYSCALL(appendToFile("Gatto", "Ciaooo", strlen("Ciaooo") + 1, NULL), "ERRORE")




//
////    srand(time(NULL));
////    sleep(rand()%5);
//
//
    struct timespec abstime;
    abstime.tv_sec = time(NULL) + 10;
    SYSCALL(openConnection(argv[1], 1000, abstime), "Errore")

    puts("1. openConnection");
    puts("2. closeConnection");
    puts("3. openFile");
    puts("4. readFile");
    puts("5. readNFiles");
    puts("6. writeFile");
    puts("7. appendToFile");
    puts("8. lockFile");
    puts("9. unlockFile");
    puts("10. closeFile");
    puts("11. removeFile");
    puts("========================================");


    while(1)
    {
        int expression = 0;
        printf("expression: ");
        scanf("%d", &expression);

        int N;

        char *sockaneme = malloc(1000);
        char *pathname = malloc(1000);
        char *dirname = malloc(1000);
        char *buf = malloc(1000);
        char *readBuf = NULL;
        size_t readSize;


        switch (expression)
        {
            case 1:
                printf("sockaneme: ");
                SCANF_STRINGA(sockaneme)
                sockaneme = realloc(sockaneme, sizeof(sockaneme) + 1);
                printf("%s\n", sockaneme);
                struct timespec abstime;
                abstime.tv_sec = time(NULL) + 10;
                SYSCALL(openConnection(sockaneme, 1000, abstime), "Errore")
                break;
            case 2:
                printf("sockaneme: ");
                SCANF_STRINGA(sockaneme)
                SYSCALL(closeConnection(sockaneme), "Errore")
                break;
            case 3:
                printf("Pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);
                printf("flags: ");

                int flag;
                scanf("%d", &flag);
                printf("%s\n", pathname);
                SYSCALL(openFile(pathname, flag), "Errore")
                break;
            case 4:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                SYSCALL(readFile(pathname, &readBuf, &readSize), "Errore")
                if(readBuf != NULL)
                {
                    printf("readBuf: %s\n", readBuf);
                    free(readBuf);
                }
                break;
            case 5:
                printf("N: ");
                scanf("%d", &N);

                printf("dirname: ");
                SCANF_STRINGA(dirname)
                dirname = realloc(dirname, sizeof(dirname) + 1);

                SYSCALL(readNFiles(N, dirname), "Errore")
                break;
            case 6:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                printf("dirname: ");
                SCANF_STRINGA(dirname)
                dirname = realloc(dirname, sizeof(dirname) + 1);
                if(strcmp(dirname, "NULL") == 0)
                {
                    free(dirname);
                    dirname = NULL;
                }

                SYSCALL(writeFile(pathname, dirname), "Errore")
                break;
            case 7:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                printf("buf: ");
                SCANF_STRINGA(buf)
                buf = realloc(buf, sizeof(buf) + 1);

                printf("dirname: ");
                SCANF_STRINGA(dirname)
                dirname = realloc(dirname, sizeof(dirname) + 1);
                if(strcmp(dirname, "NULL") == 0)
                {
                    free(dirname);
                    dirname = NULL;
                }

                SYSCALL(appendToFile(pathname, buf, strlen(buf) + 1, dirname), "Errore")
                break;
            case 8:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                SYSCALL(lockFile(pathname), "Errore")
                break;
            case 9:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                SYSCALL(unlockFile(pathname), "Errore")
                break;
            case 10:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                SYSCALL(closeFile(pathname), "Errore")
                break;
            case 11:
                printf("pathname: ");
                SCANF_STRINGA(pathname)
                pathname = realloc(pathname, sizeof(pathname) + 1);

                SYSCALL(removeFile(pathname), "Errore")
                break;
        }

        free(sockaneme);
        free(pathname);
        free(dirname);
        free(buf);
    }

    return 0;
}
