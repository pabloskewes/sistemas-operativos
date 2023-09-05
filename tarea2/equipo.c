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
int n_jugadores = 0;
char **equipo_completo;
int equipo_completo_listo = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex);
    printf("Jugador %s entra al mutex, se registran %d jugadores\n", nombre,
           n_jugadores);
    printf("equipo global: [1]  ");
    print_equipo(equipo);

    if (n_jugadores == TEAM_SIZE) {
        printf("Jugador %s llega a un equipo completo, se resetea 'equipo' "
               "pero no 'equipo_completo' ya que esta variable se retorna\n",
               nombre);

        equipo_completo_listo = 0;
        reset_team(equipo);
        n_jugadores = 0;
        printf("equipo global: [2]  ");
        print_equipo(equipo);
    }

    printf("Jugador %s se registra en el equipo y aumenta n_jugadores de %d a "
           "%d\n",
           nombre, n_jugadores, n_jugadores + 1);
    equipo[n_jugadores] = nombre;
    n_jugadores++;
    printf("equipo global: [3]  ");
    print_equipo(equipo);

    while (n_jugadores < TEAM_SIZE || !equipo_completo_listo) {
        printf("Jugador %s debe esperar a que se complete el equipo\n ya que "
               "n_jugadores es %d < %d\n",
               nombre, n_jugadores, TEAM_SIZE);
        pthread_cond_wait(&cond, &mutex);
    }
    printf("Jugador %s sale del wait, n_jugadores es %d\n", nombre,
           n_jugadores);
    printf("equipo global: [4]  ");
    print_equipo(equipo);

    if (n_jugadores == TEAM_SIZE && !equipo_completo_listo) {
        // Si quiere entrar el último jugador, copiar equipo en equipo_completo
        // para que todos los jugadores lo tengan y lo retornen
        printf("Jugador %s es el último en llegar, copia 'equipo' en "
               "'equipo_completo' y resetea 'equipo' y 'n_jugadores' y "
               "despierta a todos los jugadores\n",
               nombre);

        equipo_completo_listo = 1;
        equipo_completo = copy_team(equipo);
        // reset_team(equipo);
        // n_jugadores = 0;

        printf("equipo global: [5]  ");
        print_equipo(equipo);
        printf("equipo_completo global: [1]  ");
        print_equipo(equipo_completo);
    }
    pthread_cond_broadcast(&cond);

    printf("Jugador %s sale del mutex, n_jugadores=%d\n", nombre, n_jugadores);
    printf("equipo global: [6]  ");
    print_equipo(equipo);
    printf("equipo_completo global: [2]  ");
    print_equipo(equipo_completo);

    pthread_mutex_unlock(&mutex);
    return equipo_completo;
}

void init_equipo(void) {
    equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
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

void print_state(char *nombre, char *message) {
    printf("Jugador %s: %s\n n_jugadores=%d\n equipo_completo_listo=%d\n "
           "equipo global: ",
           nombre, message, n_jugadores, equipo_completo_listo);
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
