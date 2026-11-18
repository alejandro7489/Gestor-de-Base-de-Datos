#ifndef SPOTIFY_DBMS_FRAME_H
#define SPOTIFY_DBMS_FRAME_H

#include "Pagina.h"

using namespace std;

struct Frame {
    // El identificador y los bytes pertenecen a pagina; solo son validos si ocupado.
    Pagina pagina;
    bool ocupado = false;

    explicit Frame(size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO)
        : pagina(0, tamanoPagina) {}
};

#endif
