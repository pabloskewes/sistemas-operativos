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

Cómo resolver:
Tipo que tenias que definir un char** = equipo
Eso despues dentro de la funcion le haces una referencia local
Tipo char** mi_equipo=equipo
Y los dos hint mas brigidos es que tienes que hacer una condicion cuando son 5
en el equipo haces que la variable global sea Null, tipo equipo = NULL Y el
último es que la wea no retorna hasta que mi_equipo = equipo Eso dentro de un
while

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "equipo.h"
#include "pss.h"

#define TEAM_SIZE 5
#define DEBUG 0

char **equipo;
int n_jugadores;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex);

    if (n_jugadores == TEAM_SIZE) {
        // Jugador 6 entra y aloja memoria para el equipo
        equipo = malloc(TEAM_SIZE * sizeof(char *));
        n_jugadores = 0;
    }
    char **equipo_local = equipo;

    equipo_local[n_jugadores] = nombre;
    n_jugadores++;
    print_state(nombre, "se agrega al equipo", equipo_local);

    while (!full_team(equipo_local)) {
        print_state(nombre, "espera a que se complete el equipo", equipo_local);
        pthread_cond_wait(&cond, &mutex);
    }

    print_state(nombre, "sale del while o nunca entra", equipo_local);
    if (n_jugadores == TEAM_SIZE) {
        equipo = NULL;
        print_state(nombre,
                    "es el ultimo jugador del equipo, resetea equipo global y "
                    "despierta a todos",
                    equipo_local);
        pthread_cond_broadcast(&cond);
    }

    print_state(nombre, "sale del mutex", equipo_local);
    pthread_mutex_unlock(&mutex);
    return equipo_local;
}

void init_equipo(void) {
    equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
}

void end_equipo(void) {
    free(equipo);
}

void print_equipo(char **equipo_to_print) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        printf("%s ", equipo_to_print[i]);
    }
    printf("\n");
}

int full_team(char **equipo) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        if (equipo[i] == NULL) {
            return 0;
        }
    }
    return 1;
}

#if DEBUG
void print_state(char *nombre, char *message, char **equipo_local) {
    printf("\n");
    printf("%s: %s\n", nombre, message);
    printf("n_jugadores: %d\n", n_jugadores);
    printf("equipo local: ");
    print_equipo(equipo_local);
    printf("equipo global: ");
    if (equipo == NULL) {
        printf("NULL");
    } else {
        print_equipo(equipo);
    }
    printf("\n");
}
#else
void print_state(char *nombre, char *message, char **equipo_local) {
}

#endif
