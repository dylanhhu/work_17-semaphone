#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "stuff.h"


void print_help();


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("control: one command line argument required, none provided\n");
        print_help();
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "--create") || !strcmp(argv[1], "-c")) {
        int sem = semget(KEY, 1, IPC_CREAT | IPC_EXCL | 0644);
        if (sem == -1) {
            printf("control: couldn't create semaphore (%s, %d)\n", strerror(errno), errno);
            return errno;
        }

        union semun sem_dat;
        sem_dat.val = 1;

        int ret = semctl(sem, 0, SETVAL, sem_dat);
        if (ret == -1) {
            printf("control: couldn't set semaphore value (%s, %d)\n", strerror(errno), errno);
            ret = semctl(sem, 0, IPC_RMID);  // abort by removing the semaphore created
            return errno;
        }

        int file = open("gamefile.txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (file < 0) {
            printf("control: couldn't create file (%s, %d)\n", strerror(errno), errno);
            ret = semctl(sem, 0, IPC_RMID);  // abort by removing the semaphore created
            return errno;
        }
        close(file);

        int *len_last_line;
        int smem = shmget(KEY + 1, sizeof(int), IPC_CREAT | IPC_EXCL | 0640);  // create shared memory for the length of last line
        if (smem == -1) {
            printf("control: couldn't create shared memory (%s, %d)\n", strerror(errno), errno);
            ret = semctl(sem, 0, IPC_RMID);  // abort by removing the semaphore created
            return errno;
        }
        len_last_line = shmat(smem, 0, 0);  // attach the shared memory segment
        *len_last_line = 0;  // set the shared memory to zero
        shmdt(len_last_line);  // detach the shared memory segment

        return 0;
    }
    else if (!strcmp(argv[1], "--remove") || !strcmp(argv[1], "-r")) {
        int smem = shmget(KEY + 1, 0, 0);  // grab the shared memory identifier
        if (smem == -1) {
            printf("control: couldn't get access to shared memory (%s, %d)\n", strerror(errno), errno);
            return errno;
        }
        shmctl(smem, IPC_RMID, 0);  // remove the segment

        int sem = semget(KEY, 1, 0);  // get the semaphore identifier
        if (sem == -1) {
            printf("control: couldn't get access to the semaphore (%s, %d)\n", strerror(errno), errno);
            return errno;
        }
        semctl(sem, IPC_RMID, 0);  // remove the semaphore

        int file = open("gamefile.txt", O_RDONLY);
        if (file < 0) {
            printf("control: couldn't open file (%s, %d)\n", strerror(errno), errno);
            return errno;
        }

        struct stat filestats;
        stat("gamefile.txt", &filestats);

        char *contents = malloc(filestats.st_size);
        int readres = read(file, contents, filestats.st_size);
        if (readres < 0) {
            printf("control: couldn't read from file (%s, %d)\n", strerror(errno), errno);
            return errno;
        }

        printf("Your total game log:\n");
        printf("%s\n", contents);
        close(file);
        
        return 0;
    }

    printf("control: %s argument not recognized\n", argv[1]);
    print_help();
    return EXIT_FAILURE;
}


void print_help() {
    printf("\ncontrol: controller for semaphone game\n");
    printf("Usage:\n");
    printf("\tcontrol <args>\n");
    printf("\nArguments:\n");
    printf("\t-c --create\n");
    printf("\t\tSet the game up. Creates all files and setup necessary\n");
    printf("\n");
    printf("\t-r --remove\n");
    printf("\t\tCloses the game. Removes all necessary game setup, but leaves the game file intact.\n");
    printf("\t\tPrints out the game log.\n");
}
