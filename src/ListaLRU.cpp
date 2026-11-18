#include "ListaLRU.h"

#include <stdexcept>

using namespace std;

ListaLRU::ListaLRU() = default;

void ListaLRU::registrarFrame(size_t) {
    throw logic_error("Pendiente: registrar un frame en la lista doblemente enlazada.");
}

void ListaLRU::actualizarUso(size_t) {
    throw logic_error("Pendiente: mover el frame al extremo mas reciente.");
}

optional<size_t> ListaLRU::menosReciente() const {
    throw logic_error("Pendiente: identificar el frame menos reciente.");
}
