/*
En esta tarea Ud. deberá programar un sistema de subastas para threads.
La API que Ud. debe implementar es la siguiente:
- Subasta nuevaSubasta(int n): crea un nuevo sistema para
subastar n unidades idénticas.
- void destruirSubasta(Subasta s): libera los recursos de la
subasta.
- int adjudicar(Subasta s, int *prestantes): Cierra la subasta
retornando el monto total recaudado y entregando en *prestantes
la cantidad de unidades sobrantes de la subasta, es decir las que
no se vendieron porque llegaron menos que n oferentes.
- int ofrecer(Subasta s, int precio): múltiples tareas invocan esta
función para hacer una oferta por comprar una unidad del
producto al valor precio. Esta función espera hasta que (i) la
subasta se cierre, retornando TRUE, o (ii) n otras tareas
ofrecieron un precio mayor por el producto, y en tal caso se
retorna FALSE.

Requerimientos
- Ud.debe programar las funciones pedidas y la estructura del tipo Subasta en
el archivo subasta.c.
- Ud. debe resolver esta tarea usando un mutex y múltiples condiciones
como herramienta de sincronización. No puede usar semáforos.
- Use el patrón request eficientemente: Use múltiples condiciones para
la espera en ofrecer. Debe evitar usar pthread_cond_broadcast para
despiertar a todos los threads que esperan en ofrecer. Use
pthread_cond_signal.
- Use la cola de prioridades que está programada en subasta.c.
*/

#include "subasta.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

PriQueue MakePriQueue(int size);   // Constructor
void DestroyPriQueue(PriQueue pq); // Destructor

// Las operaciones
void *PriGet(PriQueue pq); // Extraer elemento de mejor prioridad
void PriPut(PriQueue pq, void *t, double pri); // Agregar con prioridad
double PriBest(PriQueue pq);                   // Prioridad del mejor elemento
int EmptyPriQueue(PriQueue pq); // Verdadero si la cola esta vacia
int PriLength(PriQueue pq);     // Largo de la cola

// =============================================================
// Implemente a partir de aca el tipo Subasta
// =============================================================

typedef struct subasta {
    int n;       // Cantidad de unidades a subastar
    PriQueue pq; // Cola de prioridades de ofertas
    int cerrada; // Indica si la subasta esta cerrada
    pthread_mutex_t mutex;
} *Subasta;

typedef struct {
    int ready;
    pthread_cond_t cond;
    double oferta;
} Oferta;

// header de funciones auxiliares para depurar
void printQueue(PriQueue pq, char *message);
void printSubasta(Subasta s, char *message);
void printOferta(Oferta *o, char *message);

Subasta nuevaSubasta(int n) {
    Subasta s = malloc(sizeof(*s));
    s->pq = MakePriQueue(MAXQSZ);
    s->n = n;
    s->cerrada = FALSE;
    pthread_mutex_init(&s->mutex, NULL);
    return s;
}

void destruirSubasta(Subasta s) {
    DestroyPriQueue(s->pq);
    pthread_mutex_destroy(&s->mutex);
    free(s);
}

double adjudicar(Subasta s, int *prestantes) {
    pthread_mutex_lock(&s->mutex);

    s->cerrada = TRUE;
    double recaudado = 0;
    int unidades_restantes = s->n;
    while (!EmptyPriQueue(s->pq)) {
        Oferta *o = PriGet(s->pq);
        recaudado += o->oferta;
        o->ready = TRUE;
        pthread_cond_signal(&o->cond);
        unidades_restantes--;
    }

    *prestantes = unidades_restantes;
    pthread_mutex_unlock(&s->mutex);

    return recaudado;
}

int ofrecer(Subasta s, double precio) {
    pthread_mutex_lock(&s->mutex);

    // printSubasta(
    //     s, "Estado de la subasta justo antes de que entre un nuevo
    //     oferente");

    Oferta *oferente_actual = (Oferta *)malloc(sizeof(Oferta));
    oferente_actual->ready = FALSE;
    pthread_cond_init(&oferente_actual->cond, NULL);
    oferente_actual->oferta = precio;

    // printOferta(oferente_actual, "Nuevo oferente");

    double worst_offer_price = PriBest(s->pq);

    if (PriLength(s->pq) >= s->n) {
        // Si no quedan cupos, se compara la oferta con la peor oferta
        if (precio > worst_offer_price) {
            Oferta *worst_offer = PriGet(s->pq);
            worst_offer->ready = 1;
            PriPut(s->pq, oferente_actual, precio);
            pthread_cond_signal(&worst_offer->cond);
        } else {

            oferente_actual->ready = TRUE;
            pthread_cond_signal(&oferente_actual->cond);
        }
    } else {
        // Si quedan cupos, se agrega la oferta a la cola directamente
        PriPut(s->pq, oferente_actual, precio);
    }

    while (!oferente_actual->ready && !s->cerrada) {
        pthread_cond_wait(&oferente_actual->cond, &s->mutex);
    }

    pthread_mutex_unlock(&s->mutex);

    // free oferente_actual
    pthread_cond_destroy(&oferente_actual->cond);
    free(oferente_actual);

    return s->cerrada;
}

// =============================================================
// No toque nada a partir de aca: es la implementacion de la cola
// de prioridades
// =============================================================

PriQueue MakePriQueue(int maxsize) {
    PriQueue pq = malloc(sizeof(*pq));
    pq->maxsize = maxsize;
    pq->vec = malloc(sizeof(void *) * (maxsize + 1));
    pq->ofertas = malloc(sizeof(double) * (maxsize + 1));
    pq->size = 0;
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
    if (pq->size == 0)
        return NULL;
    t = pq->vec[0];
    pq->size--;
    for (k = 0; k < pq->size; k++) {
        pq->vec[k] = pq->vec[k + 1];
        pq->ofertas[k] = pq->ofertas[k + 1];
    }
    return t;
}

void PriPut(PriQueue pq, void *t, double oferta) {
    if (pq->size == pq->maxsize)
        fatalError("PriPut", "Desborde de la cola de prioridad\n");
    int k;
    for (k = pq->size - 1; k >= 0; k--) {
        if (oferta > pq->ofertas[k])
            break;
        else {
            pq->vec[k + 1] = pq->vec[k];
            pq->ofertas[k + 1] = pq->ofertas[k];
        }
    }
    pq->vec[k + 1] = t;
    pq->ofertas[k + 1] = oferta;
    pq->size++;
}

double PriBest(PriQueue pq) {
    return pq->size == 0 ? 0 : pq->ofertas[0];
}

int EmptyPriQueue(PriQueue pq) {
    return pq->size == 0;
}

int PriLength(PriQueue pq) {
    return pq->size;
}

// =============================================================
// Funciones auxiliares para depurar

void printQueue(PriQueue pq, char *message) {
    printf("%s\n", message);
    printf("Queue: ");
    for (int i = 0; i < pq->size; i++) {
        printf("%f ", pq->ofertas[i]);
    }
    printf("\n");
}

void printSubasta(Subasta s, char *message) {
    printf("%s\n", message);
    printf("Subasta: ");
    printf("n: %d, ", s->n);
    printf("cerrada: %d, ", s->cerrada);
    printQueue(s->pq, "");
    printf("\n\n");
}

void printOferta(Oferta *o, char *message) {
    printf("%s\n", message);
    printf("Oferta: ");
    printf("ready: %d, ", o->ready);
    printf("oferta: %f, ", o->oferta);
    printf("\n\n");
}
