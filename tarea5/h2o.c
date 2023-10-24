#define _XOPEN_SOURCE 500
#include "h2o.h"
#include "nKernel/nthread-impl.h"

// Variables globales
NthQueue *oxyQ;
NthQueue *hydroQ;

void initH2O(void) {
    oxyQ = nth_makeQueue();
    hydroQ = nth_makeQueue();
}

void endH2O(void) {
    nth_destroyQueue(oxyQ);
    nth_destroyQueue(hydroQ);
}

H2O *nCombineOxy(Oxygen *o, int timeout) {
    // Para crear la molecula de agua invoque makeH2O(h1, h2, o)
    START_CRITICAL;
    nThread this_th = nSelf();

    int hydro_count = nth_queueLength(hydroQ);
    if (hydro_count >= 2) {
        nThread h1_th = nth_getFront(hydroQ);
        Hydrogen *h1 = (Hydrogen *)h1_th->ptr;
        nThread h2_th = nth_getFront(hydroQ);
        Hydrogen *h2 = (Hydrogen *)h2_th->ptr;

        H2O *h2o = makeH2O(h1, h2, o);

        h1_th->ptr = h2_th->ptr = this_th->ptr = h2o;

        setReady(h1_th);
        setReady(h2_th);

    } else {
        this_th->ptr = o;
        nth_putBack(oxyQ, this_th);
        if (timeout > 0) {
            // TODO: Implementar timeout
            suspend(WAIT_H2O_TIMEOUT);
        } else {
            suspend(WAIT_H2O);
        }
    }

    schedule();
    END_CRITICAL;
    return this_th->ptr;
}

H2O *nCombineHydro(Hydrogen *h) {
    START_CRITICAL;
    nThread this_th = nSelf();

    int oxy_count = nth_queueLength(oxyQ);
    int hydro_count = nth_queueLength(hydroQ);
    if (oxy_count >= 1 && hydro_count >= 1) {
        nThread o_th = nth_getFront(oxyQ);
        Oxygen *o = (Oxygen *)o_th->ptr;
        nThread h_th = nth_getFront(hydroQ);
        Hydrogen *h2 = (Hydrogen *)h_th->ptr;

        H2O *h2o = makeH2O(h2, h, o);

        o_th->ptr = h_th->ptr = this_th->ptr = h2o;

        setReady(o_th);
        setReady(h_th);
    } else {
        this_th->ptr = h;
        nth_putBack(hydroQ, this_th);
        suspend(WAIT_PUB);
    }

    schedule();
    END_CRITICAL;
    return this_th->ptr;
}
