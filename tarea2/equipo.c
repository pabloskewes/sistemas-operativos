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
#define DEBUG 1

char **equipo;
char **equipo_completo;
int n_jugadores;
int n_jugadores_check;

long long jugador_counter;
long long equipo_counter;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex);

    int id_jugador = jugador_counter;
    int equipo_de_jugador = id_jugador / TEAM_SIZE;
    jugador_counter++;

    print_state(nombre, "entra a al mutex", id_jugador, equipo_de_jugador);

    // if (equipo_de_jugador > equipo_counter) {
    //     reset_team(equipo);
    // }
    while (equipo_de_jugador > equipo_counter || n_jugadores_check > 0) {
        print_state(nombre,
                    "es un jugador de un equipo siguiente, por lo que "
                    "espera a que se forme el equipo anterior",
                    id_jugador, equipo_de_jugador);
        pthread_cond_wait(&cond, &mutex);
    }

    if (n_jugadores == TEAM_SIZE) {
        print_state(nombre,
                    "es el primer jugador del equipo, resetea n_jugadores a 0",
                    id_jugador, equipo_de_jugador);
        reset_team(equipo);
        n_jugadores = 0;
    }

    equipo[id_jugador % TEAM_SIZE] = nombre;
    n_jugadores++;
    print_state(nombre, "se agregó al equipo", id_jugador, equipo_de_jugador);

    while (n_jugadores < TEAM_SIZE) {
        print_state(nombre, "espera a que se complete el equipo", id_jugador,
                    equipo_de_jugador);
        pthread_cond_wait(&cond, &mutex);
    }

    print_state(nombre, "sale del wait", id_jugador, equipo_de_jugador);

    if (id_jugador == (equipo_counter + 1) * TEAM_SIZE - 1) {
        print_state(nombre, "es el último jugador del equipo", id_jugador,
                    equipo_de_jugador);
        equipo_completo = copy_team(equipo);
        reset_team(equipo);
        equipo_counter++;
        n_jugadores_check = TEAM_SIZE;
        print_state(nombre,
                    "copió el equipo, lo reseteó y aumentó el counter de "
                    "equipos",
                    id_jugador, equipo_de_jugador);
    }

    print_state(nombre, "despierta a los demás jugadores", id_jugador,
                equipo_de_jugador);

    --n_jugadores_check;
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);
    return equipo_completo;
}

void init_equipo(void) {
    equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
    equipo_completo = (char **)malloc(TEAM_SIZE * sizeof(char *));
    n_jugadores = 0;
    jugador_counter = 0;
    equipo_counter = 0;
    n_jugadores_check = 0;
}

void end_equipo(void) {
    free(equipo);
    free(equipo_completo);
}

void print_equipo(char **equipo_to_print) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        printf("%s ", equipo_to_print[i]);
    }
    printf("\n");
}

#if DEBUG
void print_state(char *nombre, char *message, long long id_jugador,
                 long long id_equipo) {
    printf("\n");
    printf("%s: %s\n", nombre, message);
    printf(
        "nombre: %s | n_jugadores: %d | id_jugador: %lld | id_equipo: %lld\n",
        nombre, n_jugadores, id_jugador, id_equipo);
    printf("equipo: ");
    print_equipo(equipo);
    printf("equipo_completo global: ");
    print_equipo(equipo_completo);
    printf("current equipo: %lld\n", equipo_counter);
    printf("n_jugadores_check: %d\n", n_jugadores_check);
    printf("\n");
}
#else
void print_state(char *nombre, char *message, long long id_jugador,
                 long long id_equipo) {
}
#endif

int full_team(char **equipo) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        if (equipo[i] == NULL) {
            return 0;
        }
    }
    return 1;
}

int empty_team(char **equipo) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        if (equipo[i] != NULL) {
            return 0;
        }
    }
    return 1;
}

void reset_team(char **equipo) {
    for (int i = 0; i < TEAM_SIZE; i++) {
        equipo[i] = NULL;
    }
}

char **copy_team(char **equipo) {
    char **new_team = (char **)malloc(TEAM_SIZE * sizeof(char *));
    for (int i = 0; i < TEAM_SIZE; i++) {
        new_team[i] = equipo[i];
    }
    return new_team;
}
