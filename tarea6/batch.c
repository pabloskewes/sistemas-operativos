#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "pss.h"
#include "batch.h"
#include "spinlocks.h"

#define FALSE 0
#define TRUE 1

struct job {
  // Defina aca esta estructura
  ...
};

// Defina sus variables globales
...

void initBatch(void) {
  ...
}

void cleanBatch(void) {
  ...
}

Job *submitJob(JobFun fun, void *input) {
  ...
}

void *waitJob(Job *job) {
  ...
}

void batchServerFun(void) {
  ...
}
