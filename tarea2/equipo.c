/*
Se necesita formar equipos de 5 jugadores de baby-fútbol. Los
jugadores son representados por threads que invocan la función
hay_equipo indicando como argumento su nombre. Esta función espera
hasta que 5 jugadores hayan invocado la misma función retornando un
arreglo de 5 strings con los nombres del equipo completo. Este es un
ejemplo del código de un jugador:

void *jugador(void *ptr) {
  char *nombre= ptr;
  for (;;) {
    dormir();
    char **equipo= hay_equipo(nombre);
    jugar_baby(equipo);
    beber_cerveza();
} }

Programe la función hay_equipo. El encabezado es el siguiente:
char **hay_equipo(char *nombre);
Las funciones dormir, jugar_futbolito y beber_cerveza son dadas.
Observe que la llamada a hay_equipo espera hasta que se haya formado
un equipo con 5 jugadores. Los primeros 5 jugadores (J 1 a J5) forman el
equipo a y por lo tanto sus llamadas a hay_equipo retornan el mismo
arreglo a con los 5 nombres del equipo: “pedro”, “juan”, …, “diego”.
Los siguientes 5 jugadores (J6 a J10) forman el equipo b y sus llamadas a
hay_equipo retornan el arreglo b, distinto de a, con los nombres
“jaime”, “jorge”, …, “luis”. Resuelva el problema usando variables
globales como un mutex, una condición, etc.

Si necesita inicializar variables globales o liberar recursos, hágalo en las
funciones init_equipo y end_equipo.

Restricciones: Para programar las funciones solicitadas Ud. debe usar
un mutex y una sola condición. No está permitido usar múltiples
condiciones.

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "equipo.h"
#include "pss.h"

#define TEAM_SIZE 5

char **equipo;
int n_jugadores;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex);

    // Si al comenzar, el equipo está completo, se libera todo
    if (n_jugadores == TEAM_SIZE) {
        printf("Equipo completo, liberando todo\n");
        n_jugadores = 0;
        equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
        print_equipo(equipo);
    }

    // Se agrega el jugador al equipo
    equipo[n_jugadores] = nombre;
    printf("Jugador %s agregado al equipo\n", nombre);
    print_equipo(equipo);

    // Se incrementa el número de jugadores
    n_jugadores++;

    // Si el número de jugadores es menor al tamaño del equipo, se espera
    while (n_jugadores < TEAM_SIZE) {
        pthread_cond_wait(&cond, &mutex);
    }

    // Se notifica a todos los jugadores que el equipo está completo
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);
    return equipo;
}

void init_equipo(void) {
    equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
    n_jugadores = 0;
}

void end_equipo(void) {
    free(equipo);
}

void print_equipo(char **equipo) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        printf("%s ", equipo[i]);
    }
    printf("\n");
}
