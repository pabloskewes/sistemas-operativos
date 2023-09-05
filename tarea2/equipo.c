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
char **equipo_completo;
int equipo_completo_listo;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex);

    print_state(nombre, "entra al mutex");

    if (n_jugadores == TEAM_SIZE) {
        print_state(nombre, "n_jugadores == TEAM_SIZE");

        equipo_completo_listo = 0;
        reset_team(equipo);
        n_jugadores = 0;

        print_state(nombre, "se resetea equipo y n_jugadores");
    }

    // print_state(nombre,
    //             "(ANTES) se inserta en equipo y se aumenta n_jugadores");
    equipo[n_jugadores] = nombre;
    n_jugadores++;
    print_state(nombre, "(DESPUES) se inserta en equipo y se aumenta "
                        "n_jugadores");

    while (n_jugadores < TEAM_SIZE) {
        print_state(nombre, "entra al wait");
        pthread_cond_wait(&cond, &mutex);
    }
    print_state(nombre, "sale del wait (o no entró)");

    if (n_jugadores == TEAM_SIZE && !equipo_completo_listo) {
        print_state(nombre,
                    "n_jugadores == TEAM_SIZE && !equipo_completo_listo");

        equipo_completo_listo = 1;
        equipo_completo = copy_team(equipo);
        // reset_team(equipo);
        // n_jugadores = 0;

        print_state(nombre, "se copia equipo a equipo_completo y se marca "
                            "equipo_completo_listo");
    }

    print_state(nombre, "se despierta a todos los threads");
    pthread_cond_broadcast(&cond);

    print_state(nombre, "sale del mutex");

    pthread_mutex_unlock(&mutex);
    return equipo_completo;
}

void init_equipo(void) {
    equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
    equipo_completo = (char **)malloc(TEAM_SIZE * sizeof(char *));
    n_jugadores = 0;
    equipo_completo_listo = 0;
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

void print_state(char *nombre, char *message) {
    printf("\n");
    printf("%s: %s\n", nombre, message);
    printf("nombre: %s | n_jugadores: %d | equipo_completo_listo: %d | equipo: ", nombre, n_jugadores, equipo_completo_listo);
    print_equipo(equipo);
    printf("equipo_completo global: ");
    print_equipo(equipo_completo);
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
