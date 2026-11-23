#ifndef SPOTIFY_DBMS_BUFFER_POOL_H
#define SPOTIFY_DBMS_BUFFER_POOL_H

#include "Almacenamiento.h"
#include "Frame.h"
#include "ListaLRU.h"
#include "TablaHash.h"

#include <cstddef>
#include <optional>
#include <vector>

using namespace std;

struct EstadisticasBuffer {
    size_t hits = 0;
    size_t misses = 0;
    size_t lecturasDisco = 0; //coincide con misses por ahora
};

class BufferPool {
public:
    explicit BufferPool(size_t capacidad = 16, size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO);
    size_t capacidad() const;
    size_t tamanoPagina() const;
    EstadisticasBuffer estadisticas() const;

    size_t framesOcupados() const;
    // Copia de los frames para inspección, sin solicitar páginas al buffer.
    vector<Frame> estadoFrames() const;

    bool estaResidente(IdPagina id) const;

    const Pagina& obtenerPagina(IdPagina id, Almacenamiento& almacenamiento);

    void liberarPagina(IdPagina id);

private:
    optional<size_t> buscarFrame(IdPagina id) const;
    optional<size_t> elegirFrameParaCargar() const;
    vector<Frame> frames_;
    size_t tamanoPagina_;
    TablaHash tablaPaginas_;
    ListaLRU listaLRU_;
    EstadisticasBuffer estadisticas_;
};

#endif
