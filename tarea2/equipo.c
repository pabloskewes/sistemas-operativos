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

    if (n_jugadores == 0) {
        // Jugador 1 entra y aloja memoria para el equipo
        equipo = malloc(TEAM_SIZE * sizeof(char *));
    }

    if (n_jugadores == TEAM_SIZE) {
        // Jugador 6 entra y despierta a todos
        print_state(nombre, "es el primer jugador de un equipo nuevo, despierta a todos",
                    equipo);
        equipo = malloc(TEAM_SIZE * sizeof(char *));
    }

    char **equipo_local = equipo;

    equipo_local[n_jugadores] = nombre;
    n_jugadores++;
    print_state(nombre, "se agrega al equipo", equipo_local);

    if (n_jugadores == TEAM_SIZE) {
        equipo = NULL;
        n_jugadores = 0;
        print_state(nombre,
                    "es el ultimo jugador del equipo, resetea equipo global y "
                    "despierta a todos",
                    equipo_local);
        pthread_cond_broadcast(&cond);
    }

    while (equipo_local == equipo) {
        print_state(nombre, "espera a que se complete el equipo", equipo_local);
        pthread_cond_wait(&cond, &mutex);
    }

    print_state(nombre, "sale del while o nunca entra", equipo_local);

    print_state(nombre, "sale del mutex", equipo_local);
    pthread_mutex_unlock(&mutex);
    return equipo_local;
}

void init_equipo(void) {
    // equipo = (char **)malloc(TEAM_SIZE * sizeof(char *));
}

void end_equipo(void) {
    // for (int i = 0; i < TEAM_SIZE; i++) {
    //     free(equipo[i]);
    // }
    // free(equipo);
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
