#include <pthread.h>
#include <stdio.h>

#include "integral.h"

typedef struct {
    Funcion f;
    void *ptr;
    double xi, xf;
    int n;
    double area;
} Args;

void *thread_integral(void *ptr) {
    Args *data = (Args *)ptr;
    data->area = integral(data->f, data->ptr, data->xi, data->xf, data->n);
    return NULL;
}

double integral_par(Funcion f, void *ptr, double xi, double xf, int n, int p) {
    pthread_t threads[p];
    Args args[p];

    double step = (xf - xi) / p;
    double results[p];

    for (int i = 0; i < p; i++) {
        args[i].f = f;
        args[i].ptr = ptr;
        args[i].xi = xi + i * step;
        args[i].xf = xi + (i + 1) * step;
        args[i].n = n / p;
        pthread_create(&threads[i], NULL, thread_integral, &args[i]);
    }

    for (int i = 0; i < p; i++) {
        pthread_join(threads[i], NULL);
        results[i] = args[i].area;
    }

    double area = 0;
    for (int i = 0; i < p; i++) {
        area += results[i];
    }

    return area;
}
