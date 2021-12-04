#ifndef STUFF_H
#define STUFF_H

#define KEY 81230988

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

#endif
