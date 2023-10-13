#include <nthread.h>

#include "pss.h"
#include "h2o.h"

... defina aca sus variables globales ...

void initH2O(void) {
  ... inicialice aca las variables globales ...
}

void endH2O(void) {
  ... libere los recursos pedidos por ejemplo con malloc ...
}

H2O *nCombineOxy(Oxygen *o, int timeout) {
  ... implemente esta funcion ...
  // Para crear la molecula de agua invoque makeH2O(h1, h2, o)
}

H2O *nCombineHydro(Hydrogen *h) {
  ... implemente esta funcion ...
  // Para crear la molecula de agua invoque makeH2O(h1, h2, o)
}
