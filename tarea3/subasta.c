#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "subasta.h"

// =============================================================
// Use esta cola de prioridades para resolver el problema
// =============================================================

// Puede almacenar hasta 100 elementos.  No se necesita mas para el test.

#define MAXQSZ 100

typedef struct {
  void **vec;
  double *ofertas;
  int size, maxsize;
} *PriQueue;

PriQueue MakePriQueue(int size);    // Constructor
void DestroyPriQueue(PriQueue pq);  // Destructor

// Las operaciones
void *PriGet(PriQueue pq);          // Extraer elemento de mejor prioridad
void PriPut(PriQueue pq, void* t, double pri); // Agregar con prioridad
double PriBest(PriQueue pq);           // Prioridad del mejor elemento
int EmptyPriQueue(PriQueue pq);     // Verdadero si la cola esta vacia
int PriLength(PriQueue pq);         // Largo de la cola

// =============================================================
// Implemente a partir de aca el tipo Subasta
// =============================================================

struct subasta {
  ... defina aca la estructura de datos para la subasta ...
};

... programe aca las funciones solicitadas: nuevaSubasta, adjudicar, etc. ...

// =============================================================
// No toque nada a partir de aca: es la implementacion de la cola
// de prioridades
// =============================================================

PriQueue MakePriQueue(int maxsize) {
  PriQueue pq= malloc(sizeof(*pq));
  pq->maxsize= maxsize;
  pq->vec= malloc(sizeof(void*)*(maxsize+1));
  pq->ofertas= malloc(sizeof(double)*(maxsize+1));
  pq->size= 0;
  return pq;
}

void DestroyPriQueue(PriQueue pq) {
  free(pq->vec);
  free(pq->ofertas);
  free(pq);
}

void *PriGet(PriQueue pq) {
  void *t;
  int k;
  if (pq->size==0)
    return NULL;
  t= pq->vec[0];
  pq->size--;
  for (k= 0; k<pq->size; k++) {
    pq->vec[k]= pq->vec[k+1];
    pq->ofertas[k]= pq->ofertas[k+1];
  }
  return t;
}

void PriPut(PriQueue pq, void *t, double oferta) {
  if (pq->size==pq->maxsize)
    fatalError("PriPut", "Desborde de la cola de prioridad\n");
  int k;
  for (k= pq->size-1; k>=0; k--) {
    if (oferta > pq->ofertas[k])
      break;
    else {
      pq->vec[k+1]= pq->vec[k];
      pq->ofertas[k+1]= pq->ofertas[k];
    }
  }
  pq->vec[k+1]= t;
  pq->ofertas[k+1]= oferta;
  pq->size++;
}

double PriBest(PriQueue pq) {
  return pq->size==0 ? 0 : pq->ofertas[0];
}

int EmptyPriQueue(PriQueue pq) {
  return pq->size==0;
}

int PriLength(PriQueue pq) {
  return pq->size;
}
