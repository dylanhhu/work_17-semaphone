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
        printf("semaphone: hint: try using ./control -c to set the game up first\n");
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
        ret = semop(gamefile_sem, &gamefile_sembuf, 1);
        return errno;
    }

    int *last_line_size = shmat(last_line_size_smem, 0, 0);
    lseek(gamefile, -1 * *last_line_size, SEEK_END);  // seek to end - last line size

    // read the line
    char *last_line = malloc(*last_line_size);
    read(gamefile, last_line, (*last_line_size) - 1);

    // print the line
    printf("Last line: %s\n", last_line);

    // prompt for new line
    printf("Your new line: ");
    char *new_line = malloc(500);
    fgets(new_line, 500, stdin);

    // set last line size to the size of the new line
    *last_line_size = strlen(new_line);

    // write the new line
    int written = write(gamefile, new_line, strlen(new_line));
    if (written == -1) {
        printf("semaphone: couldn't write your line, sorry (%s, %d)\n", strerror(errno), errno);
        gamefile_sembuf.sem_op = 1;
        ret = semop(gamefile_sem, &gamefile_sembuf, 1);
        return errno;
    }

    shmdt(last_line_size);

    gamefile_sembuf.sem_op = 1;
    ret = semop(gamefile_sem, &gamefile_sembuf, 1);
    if (ret == -1) {
        printf("semaphone: couldn't change semaphore back (%s, %d)\n", strerror(errno), errno);
        return errno;
    }

    return 0;
}
