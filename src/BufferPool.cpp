#include "BufferPool.h"

#include <stdexcept>

using namespace std;

BufferPool::BufferPool(size_t capacidad, size_t tamanoPagina)
    : tamanoPagina_(tamanoPagina), tablaPaginas_(capacidad) {
    if (tamanoPagina_ == 0) {
        throw invalid_argument("El tamano de pagina debe ser mayor que cero.");
    }
    frames_.reserve(capacidad);
    for (size_t i = 0; i < capacidad; ++i) {
        frames_.emplace_back(tamanoPagina_);
    }
}

size_t BufferPool::capacidad() const {
    return frames_.size();
}

size_t BufferPool::tamanoPagina() const {
    return tamanoPagina_;
}

EstadisticasBuffer BufferPool::estadisticas() const {
    return estadisticas_;
}

const Pagina& BufferPool::obtenerPagina(IdPagina, Almacenamiento&) {
    throw logic_error("Pendiente: acceso a paginas mediante Buffer Pool y LRU.");
}
