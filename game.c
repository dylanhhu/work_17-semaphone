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


int main() {
    int gamefile_sem = semget(KEY, 1, 0);
    if (gamefile_sem == -1) {
        printf("semaphone: couldn't get access to the semaphore (%s, %d)\n", strerror(errno), errno);
        printf("semaphone: hint: try using ./contro -c to set the game up first\n");
        return errno;
    }

    printf("semaphone: Waiting for game files to open. Please wait a moment...\n");

    struct sembuf gamefile_sembuf;
    gamefile_sembuf.sem_num = 0;
    gamefile_sembuf.sem_op = -1;
    gamefile_sembuf.sem_flg = SEM_UNDO;

    int ret = semop(gamefile_sem, &gamefile_sembuf, 1);
    if (ret == -1) {
        printf("semaphone: couldn't get access to file thru semaphore (%s, %d)\n", strerror(errno), errno);
        return errno;
    }

    int gamefile = open("gamefile.txt", O_RDWR | O_APPEND);
    if (gamefile < 0) {
        printf("semaphone: couldn't open game file (%s, %d)\n", strerror(errno), errno);
        return errno;
    }

    int last_line_size_smem = shmget(KEY + 1, 0, 0);
    if (last_line_size_smem == -1) {
        printf("semaphone: couldn't get access to shared memory (%s, %d)\n", strerror(errno), errno);
        gamefile_sembuf.sem_op = 1;
        int ret = semop(gamefile_sem, &gamefile_sembuf, 1);
        return errno;
    }

    int *last_line_size = shmat(last_line_size_smem, 0, 0);

    // TODO:
    // seek to end - last line size
    // read the line
    // print the line
    // prompt for new line
    // set last line size to the size of the new line
    // write the new line

    shmdt(last_line_size);

    gamefile_sembuf.sem_op = 1;
    int ret = semop(gamefile_sem, &gamefile_sembuf, 1);
    if (ret == -1) {
        printf("semaphone: couldn't change semaphore back (%s, %d)\n", strerror(errno), errno);
        return errno;
    }

    return 0;
}
