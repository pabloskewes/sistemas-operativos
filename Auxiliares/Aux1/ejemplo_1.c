#include <stdio.h>
#include <pthread.h>

void *thread(void *ptr) {
    char* nombre  = (char*) ptr;
    printf("Thread - %s\n", nombre);
    
    return NULL;
}

int main() {
 
    pthread_t pid_1,pid_2;
    char* nombre_1 = "primero";
    char* nombre_2 = "segundo";

    pthread_create(&pid_1, NULL, thread, nombre_1);
    pthread_create(&pid_2, NULL, thread, nombre_2);

    pthread_join(pid_1, NULL);
    pthread_join(pid_2, NULL);

    return 0;
}

