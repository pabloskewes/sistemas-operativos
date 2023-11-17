#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "batch.h"
#include "pss.h"
#include "spinlocks.h"

#define FALSE 0
#define TRUE 1

#define PRINT(msg, ptr) printf("[%p]: %s\n", ptr, msg)

/*
Programe un sistema batch que permita solicitar la ejecución asíncronade jobs
por medio de la función submitJob y esperar hasta que un jobtermine con waitJob.
Este sistema se ejecuta en una máquina sinsistema operativo y por lo tanto la
única herramienta de sincronizacióndisponible son los spinlocks.  Los jobs se
atenderán en un número fijode cores con dedicación exclusiva a ejecutar jobs.
Concretamente sepide implementar la siguiente API
*/

struct job {
    int spinLock;
    JobFun f;
    void *ptr;
    void *result;
};

Queue *coresQueue;
Queue *jobsQueue;
int mutex;

/**
 @brief Inicialice en esta función la variables globales
como las 2 colas requeridas.
  */
void initBatch(void) {
    // printf("initBatch\n");
    coresQueue = makeQueue();
    jobsQueue = makeQueue();

    mutex = OPEN;
}

/**
 * @brief Libere acá los recursos globales requeridos
 */
void cleanBatch(void) {
    // printf("cleanBatch\n");
    destroyQueue(coresQueue);
    destroyQueue(jobsQueue);
}

/**
 * @brief Solicita la
invocación de (*f)(ptr) en el sistema batch. Esta función no espera.
Retorna de inmediato la dirección de un descriptor del job que será
usado posteriormente al llamar a waitJob. Los jobs se asignan a los
cores dedicados a atender jobs por orden de llegada.
  * @param fun Función a ejecutar
  * @param input Parámetro de la función
  * @return Descriptor del job
  */
Job *submitJob(JobFun fun, void *input) {
    // printf("submitJob\n");

    // if (fun == NULL) {
    //     printf("fun == NULL [Dentro de submitJob]\n");
    // } else {
    //     printf("fun != NULL [Dentro de submitJob]\n");
    // }

    Job *job = malloc(sizeof(Job));
    job->f = fun;
    job->ptr = input;
    job->result = NULL;
    job->spinLock = CLOSED;

    spinLock(&mutex);
    put(jobsQueue, job);

    // printf("jobsQueue: %p\n", jobsQueue);

    if (queueLength(coresQueue) > 0) {
        // printf("largo de la cola de cores: %d\n", queueLength(coresQueue));
        int *core = get(coresQueue);
        // printf("core tomado: %d\n", *core);
        // se despierta al core que estaba en espera de ese job
        spinUnlock(core);
    } else {
        // printf("cola de servers vacia\n");
        spinUnlock(&mutex);
    }

    // spinUnlock(&mutex);

    return job;
}

/**
 * @brief Espera si es necesario a que termine el job
especificado, entregando el resultado de la invocación de f. Con la
invocación de waitJob se dice que el job está completo y el descriptor
es liberado.
Deberá liberar con free el descriptor del job. La excepción son las
invocaciones de submitJob en donde f es NULL. En tal caso debe
liberar el descriptor en la función batchServerFun porque no habrá
waitJob asociado
  * @param job Descriptor del job
  * @return Resultado de la invocación de f
  */
void *waitJob(Job *job) {
    // printf("waitJob\n");
    spinLock(&job->spinLock);
    void *result = job->result;
    free(job);
    // printf("result: %p\n", result);

    return result;
}

/**
 * @brief La función que ejecuta cada uno de los
cores dedicados a la ejecución de los jobs. Debe ejecutar un ciclo en
donde (i) espera la llegada de un job si no hay ninguno en espera, (ii)
ejecuta el job que lleva más tiempo en espera, y (iii) despierta al core
que estaba en espera de ese job.
Si la cola de jobs está vacía, cree un
spinlock, agréguelo a la cola de cores y espere hasta que la llegada de
un job lo despierte. Una vez que la cola no esté vacía, extraiga un job
y ejecútelo. Al terminar despierte al core que esperaba ese job
abriendo el spinlock incluido en el descriptor de job. Repita lo
mismo hasta que reciba un job en donde la función f es NULL. En
tal caso libere ese descriptor de job y retorne de la función
batchServerFun.
  */
void batchServerFun(void) {
    while (TRUE) {
        spinLock(&mutex);
        if (queueLength(jobsQueue) == 0) {
            int spinLockCore = CLOSED;

            // PRINT("queueLength == 0", (void *) &spinLockCore);
            put(coresQueue, &spinLockCore);
            // PRINT("queueLength after put", (void *) &spinLockCore);

            // esperar hasta que la llegada de un job lo despierte
            spinUnlock(&mutex);
            spinLock(&spinLockCore);
        }

        spinUnlock(&mutex);

        Job *job = get(jobsQueue);
        // printf("job->spinLock: %d\n", job->spinLock);
        if (job == NULL) {
            return;
        }

        if (job->f == NULL) {
            // printf("job->f == NULL [Dentro de batchServerFun]\n");
            free(job);
            return;
        }

        job->result = job->f(job->ptr);

        // se despierta al core que estaba en espera de ese job
        spinUnlock(&job->spinLock);
    }
}
