#define _DEFAULT_SOURCE 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include <nthread.h>

#include "pss.h"
#include "h2o.h"


#define PAUSE 300000
#define NH2O 10

#ifdef OPT
#define NOXY 50
#define NATOM 25000
#else
#define NOXY 20
#define NATOM 10000
#endif

// Ver el nMain al final

void fatalError(char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
  fflush(stdout);
  exit(1);
}

// typedef struct h2o H2O;
struct h2o {
  Hydrogen *h1, *h2;
  Oxygen *o;
};

// typedef struct hydrogen Hydrogen;
struct hydrogen {
  long i;
};

// typedef struct Oxygen Oxygen;
struct oxygen {
  long l;
};

typedef struct {
  pthread_mutex_t m;
  int cnt;
  int fill[64];
} Ctrl;

typedef struct {
  Ctrl *pctl;
  unsigned crcH;
  int timeout;
} Args;

static void *oxygen(void *ptrO) {
  H2O *agua= combineOxy(ptrO);
  return agua;
}

static void *hydrogen(void *ptrH) {
  H2O *agua= combineHydro(ptrH);
  return agua;
}

H2O *makeH2O(Hydrogen *h1, Hydrogen *h2, Oxygen *o) {
  H2O *agua= malloc(sizeof(*agua));
  agua->h1= h1;
  agua->h2= h2;
  agua->o= o;
  return agua;
}

static void *oxyProd(void *ptr) {
  Args *pa= ptr;
  pa->crcH= 0;
  for (;;) {
    pthread_mutex_lock(&pa->pctl->m);
    if (pa->pctl->cnt==0) {
      pthread_mutex_unlock(&pa->pctl->m);
      break;
    }
    pa->pctl->cnt--;
    pthread_mutex_unlock(&pa->pctl->m);
    Oxygen *o= malloc(sizeof(*o));
    o->l= random();
    H2O *agua= nCombineOxy(o, pa->timeout);
    pa->crcH += agua->h1->i;
    pa->crcH += agua->h2->i;
    free(o); free(agua->h1); free(agua->h2);
    free(agua);
  }
  return NULL;
}

static void *hydroProd(void *ptr) {
  Args *pa= ptr;
  pa->crcH= 0;
  for(;;) {
    pthread_mutex_lock(&pa->pctl->m);
    if (pa->pctl->cnt==0) {
      pthread_mutex_unlock(&pa->pctl->m);
      break;
    }
    pa->pctl->cnt--;
    pthread_mutex_unlock(&pa->pctl->m);
    Hydrogen *h= malloc(sizeof(*h));
    h->i= random();
    pa->crcH = pa->crcH + h->i;
    nCombineHydro(h);
  }
  return NULL;
}

static void testUnitario() {
  Oxygen o1, o2;
  Hydrogen h1, h2, h3, h4;
  pthread_t H1, H2, H3, H4, O1, O2;
  void *v1, *v2, *v3, *v4, *v5, *v6;
  pthread_create(&H1, NULL, hydrogen, &h1);
  usleep(PAUSE);
  pthread_create(&O1, NULL, oxygen, &o1);
  usleep(PAUSE);
  pthread_create(&O2, NULL, oxygen, &o2);
  usleep(PAUSE);
  pthread_create(&H2, NULL, hydrogen, &h2);
  usleep(PAUSE);
  pthread_create(&H3, NULL, hydrogen, &h3);
  usleep(PAUSE);
  pthread_create(&H4, NULL, hydrogen, &h4);
  pthread_join(H1, &v1);
  pthread_join(O1, &v2);
  pthread_join(H2, &v3);
  pthread_join(H3, &v4);
  pthread_join(H4, &v5);
  pthread_join(O2, &v6);
  H2O *agua= v1;
  if (agua->h1 != &h1 || agua->h2 != &h2 || agua->o != &o1)
    fatalError("primera molecula mal formada\n");
  if (agua!=v2 || agua!=v3)
    fatalError("no retorna la misma 1era. molecula en todas las llamadas\n");
  free(agua);
  H2O *agua2= v4;
  if (agua2->h1 != &h3 || agua2->h2 != &h4 || agua2->o != &o2)
    fatalError("segunda molecula mal formada\n");
  if (agua2!=v5 || agua2!=v6)
    fatalError("no retorna la misma 2da. molecula en todas las llamadas\n");
  free(agua2);
  printf("Test aprobado\n");
}

static void *hydroOrdenado(void *ptr) {
  Hydrogen *h= ptr;
  if (h->i<0)
    fatalError("Este atomo debio tener i>=0\n");
  if (h[2*NH2O].i!=-1)
    fatalError("Este atomo debio tener i==-1\n");
  combineHydro(h);
  combineHydro(&h[2*NH2O]);
  return NULL;
}

static void testOrden() {
  Hydrogen h[2*NH2O*2];
  Oxygen o[2*NH2O];
  pthread_t hydroTasks[2*NH2O];
  // Se inicializan atomos de hidrogeno y oxigeno para NH2O moleculas
  // con numeros de serie 0, 1, 2, ...
  for (int i= 0; i<NH2O; i++) {
    h[2*i].i= i;
    h[2*i+1].i= i;
    o[i].l= i;
  }
  // Se inicializan atomos de hidrogeno y oxigeno para NH2O moleculas
  // sin identificacion
  for (int i= NH2O; i<2*NH2O; i++) {
    h[2*i].i= -1;
    h[2*i+1].i= -1;
    o[i].l= -1;
  }
  // Se lanzan 2*NH2O threads que combinan cada uno 2 atomos de hidrogeno
  for (int i= 0; i<NH2O; i++) {
    pthread_create(&hydroTasks[2*i], NULL, hydroOrdenado, &h[2*i]);
    pthread_create(&hydroTasks[2*i+1], NULL, hydroOrdenado, &h[2*i+1]);
    usleep(PAUSE); // Es crucial para forzar el orden de llegada
  }
  // Ahora combinamos con NH2O atomos de oxigeno verificando orden
  for (int i= 0; i<NH2O; i++) {
    H2O *h2o= combineOxy(&o[i]);
    // El oxigeno i debe combinarse con hidrogeno con nro. de serie i
    if (h2o->h1->i != h2o->o->l   ||   h2o->h2->i != h2o->o->l)
      fatalError("Orden no es respetado\n");
    free(h2o);
  }
  // Ahora combinamos con NH2O atomos de oxigeno sin verificar orden alguno
  for (int i= NH2O; i<2*NH2O; i++) {
    H2O *h2o= combineOxy(&o[i]);
    free(h2o);
  }
  for (int i= 0; i<NH2O; i++) {
    pthread_join(hydroTasks[2*i], NULL);
    pthread_join(hydroTasks[2*i+1], NULL);
  }
  printf("Test aprobado\n");
}

static void testRobustez(int timeout) {
  Ctrl hctl, octl;
  octl.cnt= NATOM;
  hctl.cnt= 2*NATOM;
  pthread_mutex_init(&hctl.m, 0);
  pthread_mutex_init(&octl.m, 0);

  Args harg= {&hctl, 0}, oarg= {&octl, 0, timeout};
  pthread_t oxy_th[NOXY], hydro_th[2*NOXY];
  Args      oxy_a[NOXY], hydro_a[2*NOXY];
  unsigned crcH= 0, crcO=0;

  for (int k=0; k<NOXY; k++) {
    oxy_a[k]= oarg;
    hydro_a[2*k]= harg;
    hydro_a[2*k+1]= harg;
    pthread_create(&oxy_th[k], NULL, oxyProd, &oxy_a[k]);
    pthread_create(&hydro_th[2*k], NULL, hydroProd, &hydro_a[2*k]);
    pthread_create(&hydro_th[2*k+1], NULL, hydroProd, &hydro_a[2*k+1]);
  }

  for (int k=0; k<NOXY; k++) {
    pthread_join(oxy_th[k], NULL);
    crcO += oxy_a[k].crcH;
    pthread_join(hydro_th[2*k], NULL);
    pthread_join(hydro_th[2*k+1], NULL);
    crcH += hydro_a[2*k].crcH + hydro_a[2*k+1].crcH;
  }

  if (crcH!=crcO) {
    fprintf(stderr, "La suma de los hidrogenos no coincide con la calculada "
            "al combinar con oxigenos\n");
    exit(1);
  }
  printf("Test aprobado\n");
}

/****************************************************
 * Tests de timeouts
 ****************************************************/

typedef struct {
  void *oxy;
  int timeout;
} Param;

static void *oxygenTO(void *ptr) {
  Param *pparm= ptr;
  H2O *ph2o= nCombineOxy(pparm->oxy, pparm->timeout);
  free(pparm);
  return ph2o;
}

static pthread_t createOxygen(Oxygen *oxy, int timeout) {
  pthread_t t;
  Param *pparm= malloc(sizeof(Param));
  pparm->oxy= oxy;
  pparm->timeout= timeout;
  pthread_create(&t, NULL, oxygenTO, pparm);
  return t;
}

static void *hydrogenTO(void *ptr) {
  return nCombineHydro(ptr);
}

static pthread_t createHydro(Hydrogen *hydro) {
  pthread_t t;
  pthread_create(&t, NULL, hydrogenTO, hydro);
  return t;
}

void testTimeouts() {
  /* Test de prueba: semantica con timeouts */
  {
    printf("\nVerifica que el timeout expire en 300 miliseg. \n");
    printf("con nCombineOxy cuando no hay ningun hidrogeno\n");
    Oxygen o;
    void *ph2o;
    int initime= nGetTime();
    printf("Creando atomo de oxigeno con timeout 300 en T=0\n");
    pthread_t oxyThr= createOxygen(&o, 300);
    pthread_join(oxyThr, &ph2o);
    int elapsed= nGetTime()-initime;
    if (ph2o!=NULL)
      fatalError("No debio haber formado una molecula de agua\n");
    printf("El timeout fue de %d milisegundos\n", elapsed);
    if (elapsed<299)
      fatalError("El timeout fue de menos de 300 miliseg.\n");
    if (elapsed>600)
      fatalError("El timeout tomo demasiado tiempo\n");
    printf("Aprobado\n");
  }

  {
    printf("\nComo antes, pero con un hidrogeno\n");
    Hydrogen h;
    printf("Creando atomo de hidrogeno\n");
    pthread_t hydroThr= createHydro(&h);
    nSleepMillis(300);
    Oxygen o;
    void *ph2o;
    int initime= nGetTime();
    printf("Creando atomo de oxigeno con timeout 300 en T=0\n");
    pthread_t oxyThr= createOxygen(&o, 300);
    pthread_join(oxyThr, &ph2o);
    int elapsed= nGetTime()-initime;
    if (ph2o!=NULL)
      fatalError("No debio haber formado una molecula de agua\n");
    printf("El timeout fue de %d milisegundos\n", elapsed);
    if (elapsed<299)
      fatalError("El timeout fue de menos de 300 miliseg.\n");
    if (elapsed>600)
      fatalError("El timeout tomo demasiado tiempo\n");
    printf("Aprobado\n");

    printf("\nVerifica que timeout de 600 milisegundos no expire\n");
    printf("Debe formar la molecula en 300 milisegundos\n");
    initime= nGetTime();
    printf("Creando atomo de oxigeno con timeout 600 en T=0\n");
    oxyThr= createOxygen(&o, 600);
    nSleepMillis(300);
    Hydrogen h2;
    printf("Creando atomo de hidrogeno en T=%d\n", nGetTime()-initime);
    pthread_t hydroThr2= createHydro(&h2);
    pthread_join(oxyThr, &ph2o);
    elapsed= nGetTime()-initime;
    if (ph2o==NULL)
      fatalError("Si debio haber formado una molecula de agua\n");
    printf("Oxigeno formo molecula despues de %d milisegundos\n", elapsed);
    if (elapsed>600)
      fatalError("Se demoro demasiado en formar la molecula\n");
    void *phydroH2o, *phydroH2o2;
    pthread_join(hydroThr, &phydroH2o);
    pthread_join(hydroThr2, &phydroH2o2);
    H2O *agua= ph2o;
    if ( ! ((agua->h1==&h && agua->h2==&h2) ||
            (agua->h1==&h2 && agua->h2==&h) ) ||
         ph2o!=phydroH2o ||
         ph2o!=phydroH2o2 )
      fatalError("La molecula es inconsistente\n");
    free(ph2o);
    printf("Aprobado\n");
  }

  {
    printf("\nVerifica que el timeout expire con el primer oxigeno\n");
    printf("pero no expire con el segundo oxigeno\n");
    printf("Creando atomo de hidrogeno\n");
    Hydrogen h;
    pthread_t hydroThr= createHydro(&h);

    // printf("Debe formar la molecula en 300 milisegundos\n");
    Oxygen o;
    int initime= nGetTime();
    printf("Creando 1er. atomo de oxigeno con timeout 300 en T=0\n");
    pthread_t oxyThr= createOxygen(&o, 300);
    nSleepMillis(100);
    Oxygen o2;
    int initime2= nGetTime();
    printf("Creando 2do. atomo de oxigeno con timeout 1000 en T=%d\n",
           initime2-initime);
    pthread_t oxyThr2= createOxygen(&o2, 1000);
    void *ph2o;
    pthread_join(oxyThr, &ph2o);
    int elapsed= nGetTime()-initime;
    if (ph2o!=NULL)
      fatalError("No debio haber formado una molecula de agua\n");
    printf("El timeout del primer oxigeno fue de %d milisegundos\n", elapsed);
    if (elapsed<299)
      fatalError("El timeout fue de menos de 300 miliseg.\n");
    if (elapsed>600)
      fatalError("El timeout tomo demasiado tiempo\n");

    nSleepMillis(100);
    Hydrogen h2;
    printf("Creando atomo de hidrogeno en T=%d\n", nGetTime()-initime);
    pthread_t hydroThr2= createHydro(&h2);
    pthread_join(oxyThr2, &ph2o);
    elapsed= nGetTime()-initime2;
    if (ph2o==NULL)
      fatalError("El segundo oxigeno si debio haber formado una molecula\n");
    printf("El segundo oxigeno formo molecula despues de %d milisegundos\n",
           elapsed);
    if (elapsed>600)
      fatalError("Se demoro demasiado en formar la molecula\n");
    void *phydroH2o, *phydroH2o2;
    pthread_join(hydroThr, &phydroH2o);
    pthread_join(hydroThr2, &phydroH2o2);
    H2O *agua= ph2o;
    if ( ! ((agua->h1==&h && agua->h2==&h2) ||
            (agua->h1==&h2 && agua->h2==&h) ) ||
         ph2o!=phydroH2o ||
         ph2o!=phydroH2o2 )
      fatalError("La molecula es inconsistente\n");
    free(ph2o);
    printf("Aprobado\n");
  }

  {
    printf("\nVerifica que el timeout expire con el segundo oxigeno\n");
    printf("pero no con el primer oxigeno\n");
    printf("Creando atomo de hidrogeno\n");
    Hydrogen h;
    pthread_t hydroThr= createHydro(&h);

    // printf("Debe formar la molecula en 600 milisegundos\n");
    Oxygen o;
    int initime= nGetTime();
    printf("Creando 1er. atomo de oxigeno con timeout 1000 en T=0\n");
    pthread_t oxyThr= createOxygen(&o, 1000);
    nSleepMillis(100);
    Oxygen o2;
    int initime2= nGetTime();
    printf("Creando 2do. atomo de oxigeno con timeout 300 en T=%d\n",
           initime2-initime);
    pthread_t oxyThr2= createOxygen(&o2, 300);
    void *ph2o;
    pthread_join(oxyThr2, &ph2o);
    int elapsed= nGetTime()-initime2;
    if (ph2o!=NULL)
      fatalError("El segundo oxigeno no debio haber formado una molecula\n");
    printf("El timeout del segundo oxigeno fue de %d milisegundos\n", elapsed);
    if (elapsed<299)
      fatalError("El timeout fue de menos de 300 miliseg.\n");
    if (elapsed>600)
      fatalError("El timeout tomo demasiado tiempo\n");

    nSleepMillis(100);
    Hydrogen h2;
    printf("Creando atomo de hidrogeno en T=%d\n", nGetTime()-initime);
    pthread_t hydroThr2= createHydro(&h2);
    pthread_join(oxyThr, &ph2o);
    elapsed= nGetTime()-initime;
    if (ph2o==NULL)
      fatalError("El primer oxigeno si debio haber formado una molecula\n");
    printf("El primer oxigeno formo molecula despues de %d milisegundos\n",
           elapsed);
    if (elapsed>700)
      fatalError("Se demoro demasiado en formar la molecula\n");
    void *phydroH2o, *phydroH2o2;
    pthread_join(hydroThr, &phydroH2o);
    pthread_join(hydroThr2, &phydroH2o2);
    H2O *agua= ph2o;
    if ( ! ((agua->h1==&h && agua->h2==&h2) ||
            (agua->h1==&h2 && agua->h2==&h) ) ||
         ph2o!=phydroH2o ||
         ph2o!=phydroH2o2 )
      fatalError("La molecula es inconsistente\n");
    free(ph2o);
    printf("Aprobado\n");
  }
}

/****************************************************
 * Programa principal
 ****************************************************/

int run_thr= 1;

int main(int argc, char **argv) {
  initH2O();

  printf("Test de prueba de semantica sin timeouts\n");
  testUnitario();

  printf("Test de prueba de semantica de timeouts\n");
  testTimeouts();

  printf("Test del orden de formacion de las moleculas\n");
  testOrden();

  printf("Test de robustez sin timeout\n");
  testRobustez(-1);

  printf("Test de robustez con timeouts que no expiran\n");
  testRobustez(3600*1000);

  printf("Felicitaciones: todos los tests aprobados\n");

  endH2O();
  return 0;
}

