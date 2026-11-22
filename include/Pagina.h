#ifndef SPOTIFY_DBMS_PAGINA_H
#define SPOTIFY_DBMS_PAGINA_H

#include "Cancion.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

using namespace std;

using IdPagina = size_t;
inline constexpr size_t TAMANO_PAGINA_PREDETERMINADO = 4096;

struct Pagina {
    IdPagina id;
    vector<byte> datos;

    explicit Pagina(IdPagina idPagina = 0,
                    size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO)
        : id(idPagina), datos(tamanoPagina) {
        if (tamanoPagina == 0) {
            throw invalid_argument("El tamano de pagina debe ser mayor que cero.");
        }
    }

    bool agregarCancion(const Cancion& cancion);
    vector<Cancion> leerCanciones() const;
};

#endif
