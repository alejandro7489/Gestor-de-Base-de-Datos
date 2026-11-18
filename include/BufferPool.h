#ifndef SPOTIFY_DBMS_BUFFER_POOL_H
#define SPOTIFY_DBMS_BUFFER_POOL_H

#include "Almacenamiento.h"
#include "Frame.h"
#include "ListaLRU.h"
#include "TablaHash.h"

#include <cstddef>
#include <vector>

using namespace std;

struct EstadisticasBuffer {
    size_t hits = 0;
    size_t misses = 0;
};

class BufferPool {
public:
    explicit BufferPool(size_t capacidad = 16,
                        size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO);
    size_t capacidad() const;
    size_t tamanoPagina() const;
    EstadisticasBuffer estadisticas() const;

    // Pendiente: consultar la tabla, cargar del almacenamiento y aplicar LRU.
    const Pagina& obtenerPagina(IdPagina id, Almacenamiento& almacenamiento);

private:
    vector<Frame> frames_;
    size_t tamanoPagina_;
    TablaHash tablaPaginas_;
    ListaLRU listaLRU_;
    EstadisticasBuffer estadisticas_;
};

#endif
