#include "BufferPool.h"

#include <stdexcept>
#include <string>
#include <utility>

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

size_t BufferPool::framesOcupados() const {
    size_t cantidad = 0;
    for (const Frame& frame : frames_) {
        if (frame.ocupado) {
            ++cantidad;
        }
    }
    return cantidad;
}

vector<Frame> BufferPool::estadoFrames() const {
    return frames_;
}

bool BufferPool::estaResidente(IdPagina id) const {
    return buscarFrame(id).has_value();
}

optional<size_t> BufferPool::buscarFrame(IdPagina id) const {
    // Un frame libre no tiene pagina valida, por eso se mira ocupado primero.
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].ocupado && frames_[i].pagina.id == id) {
            return i;
        }
    }
    return nullopt;
}

optional<size_t> BufferPool::elegirFrameParaCargar() const {
    // 1) Preferir un frame vacio.
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (!frames_[i].ocupado) {
            return i;
        }
    }
    // 2) Sin LRU: el primer frame limpio y sin fijar, recorriendo por indice.
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].pinCount == 0 && !frames_[i].dirtyBit) {
            return i;
        }
    }
    // 3) Todos fijados (o sucios): no hay victima elegible.
    return nullopt;
}

const Pagina& BufferPool::obtenerPagina(IdPagina id, Almacenamiento& almacenamiento) {
    // HIT: la pagina ya esta en RAM. No se toca el disco.
    if (const optional<size_t> residente = buscarFrame(id)) {
        Frame& frame = frames_[*residente];
        ++frame.pinCount;
        ++estadisticas_.hits;
        return frame.pagina;
    }

    // MISS: primero comprobar que existe donde guardar la pagina.
    const optional<size_t> destino = elegirFrameParaCargar();
    if (!destino) {
        throw runtime_error("No hay frames disponibles: todas las paginas estan fijadas.");
    }

    Pagina leida = almacenamiento.leerPagina(id);
    if (leida.id != id) {
        throw runtime_error("El almacenamiento devolvio la pagina " + to_string(leida.id) +
                            " al pedir la pagina " + to_string(id) + ".");
    }
    if (leida.datos.size() != tamanoPagina_) {
        throw runtime_error("La pagina " + to_string(id) +
                            " no tiene el tamano de pagina del buffer.");
    }

    // La lectura salio bien: recien ahora se modifica el frame y se cuenta.
    Frame& frame = frames_[*destino];
    frame.pagina = move(leida);
    frame.ocupado = true;
    frame.pinCount = 1;
    frame.dirtyBit = false;
    ++estadisticas_.misses;
    ++estadisticas_.lecturasDisco;
    return frame.pagina;
}

void BufferPool::liberarPagina(IdPagina id) {
    const optional<size_t> residente = buscarFrame(id);
    if (!residente) {
        throw logic_error("No se puede liberar la pagina " + to_string(id) +
                          ": no esta en el buffer.");
    }
    Frame& frame = frames_[*residente];
    if (frame.pinCount == 0) {
        throw logic_error("No se puede liberar la pagina " + to_string(id) +
                          ": no tiene solicitudes pendientes.");
    }
    --frame.pinCount;
}

