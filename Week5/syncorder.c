#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "list.h"

pthread_mutex_t mut;
pthread_cond_t myCond=PTHREAD_COND_INITIALIZER;

int fFlag=0;
int lFlag=0;
int oFlag=0;
int reihe=0;

int write_buffer (long thread, char *buffer, int len){
    while (thread!= reihe){
        pthread_mutex_lock(&mut);
        pthread_cond_wait(&myCond, &mut);
        pthread_mutex_unlock(&mut);
    }
    return write(fileno(stdout), buffer, len);
}

void *thread_job(void *threadNumber) {

    int myNumber= *((int *)threadNumber);
    //printf("myNum: %d\n", myNumber);
    char fileName[20];
    sprintf(fileName, "%d", myNumber);
    strcat(fileName, ".txt");
    int file=open(fileName, O_RDONLY);
    if (file<0){
        perror("Cannot open file");
        return NULL;
    }
    char buffer[64];
    int i=0;
    ssize_t bytesRead;
    if (fFlag){
        pthread_mutex_lock(&mut);
        while((bytesRead=read(file, buffer, 64))){
            char prefix[20];
            sprintf(prefix, "[%02d] %03d\t", myNumber, i);
            if (write(fileno(stdout), prefix, strlen(prefix))==-1){
                perror("There was an error while writing to stdout");
                abort();
            }
            write(fileno(stdout), buffer, (int) bytesRead);
            write(fileno(stdout), "\n", 1);
            i++;
        }
        pthread_mutex_unlock(&mut);
    }
    else if (lFlag){
        while((bytesRead=read(file, buffer, 64))){
            char prefix[20];
            sprintf(prefix, "[%02d] %03d\t", myNumber, i);
            pthread_mutex_lock(&mut);
            if (write(fileno(stdout), prefix, strlen(prefix))==-1){
                perror("There was an error while writing to stdout");
                abort();
            }
            write(fileno(stdout), buffer, (int) bytesRead);
            write(fileno(stdout), "\n", 1);
            pthread_mutex_unlock(&mut);
            i++;
        }
    }
    else if (oFlag){
        while((bytesRead=read(file, buffer, 64))){
            char prefix[20];
            sprintf(prefix, "[%02d] %03d\t", myNumber, i);
            if (write_buffer(myNumber, prefix, strlen(prefix))==-1){
                perror("There was an error while writing to stdout");
                abort();
            }
            write_buffer(myNumber, buffer, (int) bytesRead);
            write_buffer(myNumber, "\n", 1);
            i++;
        }
        pthread_mutex_lock(&mut);
        reihe++;
        pthread_cond_broadcast(&myCond);
        pthread_mutex_unlock(&mut);
    }
    else{
        while((bytesRead=read(file, buffer, 64))){
            char prefix[20];
            sprintf(prefix, "[%02d] %03d\t", myNumber, i);
            if (write(fileno(stdout), prefix, strlen(prefix)+1)==-1){
                perror("There was an error while writing to stdout");
                abort();
            }
            write(fileno(stdout), buffer, (int) bytesRead);
            write(fileno(stdout), "\n", 1);
            i++;
        }
    }
    close(file);
    return NULL;
}

int main(int argc, char *argv[], char *envp[]) {
    int numThreads = 1;

    int c;

    while ((c = getopt(argc, argv, "n:lfo")) != -1) {
        switch (c) {
            case 'n':
                numThreads=atoi(optarg);
                if (numThreads>10 &&numThreads<=0){
                    perror("Invalid thread number: ");
                    abort();
                }
                break;
            case 'l':
                lFlag=1;
                break;
            case 'f':
                fFlag=1;
                break;
            case 'o':
                oFlag=1;
                break;
            case '?':
                if (optopt == 'n')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort();
        }
    }

    if (lFlag+oFlag+fFlag>=2){
        printf("Only one of the flags is accepted");
        abort();
    }

    list_t *thread_list = list_init();
    //this is a very subltle bug, we need a new variable for laufendeNummer, because we can't guarantee when the thread will deference the pointer, leading to false number passed
    //Variables declared inside a loop won't be repeatedly allocated
    //int *threadNumber=calloc(10, sizeof(int));
    int threadNumber[10];


    //printf("numThrs: %d\n", numThreads);
    pthread_t threadID[10];
    if (pthread_mutex_init(&mut, NULL)!=0){
        perror("Mutex creation failed: ");
        return 1;
    }

    for (int laufendeNummer = 0; laufendeNummer < numThreads; ++laufendeNummer) {
        threadNumber[laufendeNummer]=laufendeNummer;
        //printf("thrNum: %d, laufNum: %d\n", *threadNumber[laufendeNummer], laufendeNummer);
        if(pthread_create(threadID+laufendeNummer, NULL, &thread_job, threadNumber+laufendeNummer)!=0){
            //fprint(stderr, "Could not create ")
            perror("Error: ");
        } else {
            //printf("Thread successfully created.\n");
        };
        list_append(thread_list, threadID+laufendeNummer);
    }

    for (list_elem *curr = thread_list->first; curr; curr = curr->next) {
        pthread_join(*((pthread_t *) curr->data), NULL);
    }

    list_finit(thread_list);
    return 0;
}
