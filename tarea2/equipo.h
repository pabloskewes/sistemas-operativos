char **hay_equipo(char *nombre);
void init_equipo(void);
void end_equipo(void);
void print_equipo(char **equipo);
void print_state(char *nombre, char *message, long long id_jugador,
                 long long id_equipo);
int full_team(char **equipo);
int empty_team(char **equipo);
void reset_team(char **equipo);
char **copy_team(char **equipo);