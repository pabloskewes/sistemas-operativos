#define _DEFAULT_SOURCE 1
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

#include "pss.h"
#include "batch.h"
#include "spinlocks.h"

#define STEP 400

#define FALSE 0
#define TRUE 1

#pragma GCC diagnostic ignored "-Wunused-function"

// ==================================================
// Funcion que entrega los milisegundos transcurridos desde
// que se invoco resetTime()

static int time0= 0;

static int getTime0() {
    struct timeval Timeval;
    gettimeofday(&Timeval, NULL);
    return Timeval.tv_sec*1000+Timeval.tv_usec/1000;
}

static void resetTime() {
  time0= getTime0();
}

static int getTime() {
  return getTime0()-time0;
}

// ==================================================
// Funcion que espera que transcurran milis milisegundos

static void esperar(useconds_t milis) {
  usleep(milis*1000);
}

// ==================================================
// Funcion para reportar un error y detener la ejecucion

void fatalError( char *procname, char *format, ... ) {
  va_list ap;

  fprintf(stderr,"Error Fatal en la rutina %s y la tarea \n", procname);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(1); /* shutdown */
}

// ==================================================
// Funciones para crear los cores virtuales

static pthread_t *tasks;
static int ntasks;
static pthread_mutex_t m= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond= PTHREAD_COND_INITIALIZER;

static void *coreFun(void *ptr) {
  batchServerFun();
  return NULL;
}

static void startBatch(int n) {
  initBatch();
  ntasks= n;
  tasks= malloc(n*sizeof(pthread_t));
  for (int i= 0; i<n; i++) {
    if (pthread_create(&tasks[i], NULL, coreFun, (void*)(intptr_t)i)!=0) {
      perror("pthread_create");
      exit(1);
    }
  }
}

static void stopBatch() {
  for (int i= 0; i<ntasks; i++)
    submitJob(NULL, NULL);
  for (int i= 0; i<ntasks; i++)
    pthread_join(tasks[i], NULL);
  cleanBatch();
  free(tasks);
}

// ==================================================

typedef struct {
  int delay;
  void *tag;
  int startTime;
} Test;

static int terminados;
static int lanzados;
static int chequeados;
static int waitTerminados;

static void *testFun(void *ptr) {
  Test *test= ptr;
  test->startTime= getTime();
  if (test->delay!=0)
    esperar(test->delay);
  else {
    pthread_mutex_lock(&m);
    terminados++;
    int punto= terminados%1000==0;
    pthread_mutex_unlock(&m);
    if (punto) printf(".");
  }
  return test;
}

static Job *submitTest(int delay) {
  Test *test= malloc(sizeof(Test));
  test->delay= delay;
  test->tag= test;
  return submitJob(testFun, test);
}

static void checkTest(Job *job) {
  Test *res= waitJob(job);
  if (res->tag!=res)
    fatalError("checkTest", "El resultado %p es incorrecto\n", res);
  free(res);
}

typedef struct {
  int first, step, nr;
  Job **jobs;
} SubmitParam;
  
void createSubmitThread(pthread_t *pth, void *(*fun)(void *),
         int first, int step, int nr, Job **jobs) {
  SubmitParam parm= { first, step, nr, jobs };
  SubmitParam *pparm= malloc(sizeof(SubmitParam));
  *pparm= parm;
  if (pthread_create(pth, NULL, fun, pparm)!=0) {
    perror("pthread_create");
    exit(1);
  }
}

void *submitFun(void *ptr) {
  SubmitParam *pparm= ptr;
  int first= pparm->first;
  int step= pparm->step;
  int nr= pparm->nr;
  Job **jobs= pparm->jobs;
  free(pparm);

  for (int i=first; i<nr; i+=step) {
    pthread_mutex_lock(&m);
    jobs[i]= submitTest(0);
    lanzados++;
    int dospuntos= lanzados%1000==0;
    pthread_mutex_unlock(&m);
    if (dospuntos) printf(":");
  }

  return NULL;
}

void *waitFun(void *ptr) {
  SubmitParam *pparm= ptr;
  int first= pparm->first;
  int step= pparm->step;
  int nr= pparm->nr;
  Job **jobs= pparm->jobs;
  free(pparm);

  for (int i= first; i<nr; i+=step) {
    pthread_mutex_lock(&m);
    while (jobs[i]==NULL)
      pthread_cond_wait(&cond, &m);// todavia no se crea el job, hay que esperar
    chequeados++;
    int guion= chequeados%1000==0;
    pthread_mutex_unlock(&m);
    checkTest(jobs[i]);
    if (guion) printf("-");
  }
  pthread_mutex_lock(&m);
  waitTerminados++;
  pthread_mutex_unlock(&m);
  return 0;
}

typedef struct {
  pthread_t *submitters;
  int nsubmitters;
} WaitParam;

void *waiterFun(void *ptr) {
  WaitParam *pparm= ptr;
  pthread_t *submitters= pparm->submitters;
  int nsubmitters= pparm->nsubmitters;
  free(pparm);

  for (int j= 0; j<nsubmitters; j++) {
    pthread_join(submitters[j], NULL);
  }
  printf("$");
  stopBatch();
  return 0;
}

void createWaiterThread(pthread_t *pth, pthread_t *submitters, int nsubmitters)
{
  WaitParam parm= { submitters, nsubmitters };
  WaitParam *pparm= malloc(sizeof(WaitParam));
  *pparm= parm;
  if (pthread_create(pth, NULL, waiterFun, pparm)!=0) {
    perror("pthread_create");
    exit(1);
  }
}

#ifdef SANTHR
#define BWTOP 1
#else
#define BWTOP 2
#endif

int main() {
  for (int bw= 0; bw<BWTOP; bw++) {
    if (bw==0)
      printf("Prueba con spinlocks implementados con mutex sin busywaiting\n");
    else {
      printf("\n");
      printf("===============================================\n");
      printf("Prueba con spinlocks verdaderos con busywaiting\n");
      printf("===============================================\n\n");
    }
    setBusyWaiting(bw);
    resetTime();
    printf("Test secuencial: 1 solo job a la vez\n");
    {
      startBatch(1);

      int ini= getTime();
      Job *r1= submitTest(STEP);
      Job *r2= submitTest(STEP);
      Job *r3= submitTest(STEP);

      checkTest(r1);
      checkTest(r2);
      checkTest(r3);
      int elapsed= getTime()-ini;
      printf("Tiempo transcurrido: %d milisegundos\n", elapsed);
      if (elapsed<STEP*3 || elapsed>STEP*7/2)
        fatalError("nMain",
            "Error: tiempo %d no esta entre [%d, %d]\n",
            elapsed, STEP*3,STEP*7/2);

      printf("\nDeteniendo el sistema batch\n");
      stopBatch();
    }
    printf("----------------------------------------\n\n");

    printf("Test paralelo: 2 jobs simultaneos\n");
    {
      startBatch(2);

      int ini= getTime();
      Job *r1= submitTest(STEP);
      Job *r2= submitTest(STEP);
      Job *r3= submitTest(STEP);

      checkTest(r1);
      checkTest(r2);
      checkTest(r3);
      int elapsed= getTime()-ini;
      printf("Tiempo transcurrido: %d milisegundos\n", elapsed);
      if (elapsed<STEP*2 || elapsed>STEP*5/2)
        fatalError("nMain",
            "Error: tiempo %d no esta entre [%d, %d]\n",
            elapsed, STEP*2, STEP*5/2);

      stopBatch();
    }
    printf("----------------------------------------\n\n");

    printf("Test paralelo: 100 jobs y 10 threads\n");
    {
      int nr= 100;
      int ncor= 10;
      startBatch(ncor);

      Job *jobs[nr];
      int ini= getTime();
      for (int i= 0; i<nr; i++) {
        jobs[i]= submitTest(STEP);
      }

      for (int i= 0; i<nr; i++) {
        checkTest(jobs[i]);
      }
      int elapsed= getTime()-ini;
      printf("Tiempo transcurrido: %d milisegundos\n", elapsed);
      if (elapsed<STEP*10 || elapsed>STEP*13)
        fatalError("nMain",
            "Error: tiempo %d no esta entre [%d, %d]\n",
            elapsed, STEP*10, STEP*13);

      stopBatch();
    }
    printf("----------------------------------------\n\n");

    printf("Test del orden de llegada: 10 jobs separados por %d milisegundos\n",
           STEP);
    {
      int nr= 10;
      startBatch(1);
 
      int ini= getTime();
      Job *jobs[nr];
      jobs[0]= submitTest(STEP*11);
      for (int i= 1; i<nr; i++) {
        esperar(STEP);
        jobs[i]= submitTest(STEP);
      }

      int lastStartTime= 0;
      for (int i= 0; i<nr; i++) {
        Test *test= waitJob(jobs[i]);
        if (lastStartTime>test->startTime)
          fatalError("nMain", "no se respeta orden de llegada\n");
        lastStartTime= test->startTime;
        free(test);
      }

      int elapsed= getTime()-ini;
      printf("Tiempo transcurrido: %d milisegundos\n", elapsed);
      if (elapsed<STEP*20 || elapsed>STEP*23)
        fatalError("nMain",
            "Error: tiempo %d no esta entre [%d, %d]\n",
            elapsed, STEP*20, STEP*23);

      stopBatch();
    }
    printf("----------------------------------------\n\n");

    printf("Cada '.' corresponde a 1000 jobs lanzados\n");
    printf("Cada ':' corresponde a 1000 jobs terminados\n");
    printf("Cada '-' corresponde a 1000 waitJob completados\n");
    printf("El '$' corresponde a la llamada de stopBatch\n\n");
    int njobs[]= {50, 200, 1000 };
    int ncores[]= {4, 20, 40 };

    for (int k= 0; k<3; k++) {
      int ncor= ncores[k]; // El numero de threads
      int nr= njobs[k];    // Numero de jobs
      printf("test de robustez con %d threads y %d jobs\n", ncor, nr);

      startBatch(ncor);    // Se lanza el sistema batch con ncor threads
      lanzados= 0;         // jobs creados
      terminados= 0;       // jobs terminados
      chequeados= 0;       // jobs completados
      int nsubmitters= 2*ncor; // 2*ncor tareas crearan jobs
                           // ncor tareas se encargaran de completarlos
      waitTerminados= 0;   // tareas que invocan a waitTest terminadas

      int ini= getTime(); // hora de inicio
      Job *jobs[nr];       // los nr jobs que se crearan
      for (int j= 0; j<nr; j++)
        jobs[j]= NULL;     // se necesita que partan en NULL
      printf("creando jobs\n");
      pthread_t submitters[nsubmitters], waitters[ncor];
      for (int j=0; j<nsubmitters; j++) {
        createSubmitThread(&submitters[j], submitFun, j, nsubmitters, nr, jobs);
      }
      for (int j=0; j<ncor; j++) {
        createSubmitThread(&waitters[j], waitFun, j, ncor, nr, jobs);
      }
      // Lanzamos una tarea encargada de enterrar los submitters y llamar
      // a stopBatch
      pthread_t waitsubtask;
      createWaiterThread(&waitsubtask, submitters, nsubmitters);
      // Parte truculenta: los waitters llaman a wait cuando encuentran
      // un job que aun no ha sido creado.  Cada 100 milisegundos llamamos
      // a broadcast para que vuelvan a intentar completar el job
      pthread_mutex_lock(&m);
      while (waitTerminados!=ncor) {
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&m);
        esperar(100);
        pthread_mutex_lock(&m);
      }
      pthread_mutex_unlock(&m);
      // Enterramos los waitters aqui mismo
      printf("\nesperando jobs\n");
      for (int j= 0; j<ncor; j++) {
        pthread_join(waitters[j], NULL);
      }
      pthread_join(waitsubtask, NULL);
      int elapsed= getTime() - ini;
      printf("\nTiempo transcurrido: %d milisegundos\n", elapsed);
    }
  }

  printf("Felicitaciones.  Su tarea paso todos los tests\n");
  return 0;
}
