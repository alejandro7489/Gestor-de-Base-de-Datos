#include "TablaHash.h"

#include <stdexcept>

using namespace std;

TablaHash::TablaHash(size_t capacidad) : capacidad_(capacidad) {
    if (capacidad_ == 0) {
        throw invalid_argument("La capacidad de la tabla debe ser mayor que cero.");
    }
}

size_t TablaHash::capacidad() const {
    return capacidad_;
}

void TablaHash::insertar(const string&, size_t) {
    throw logic_error("Pendiente: insercion en la tabla hash propia.");
}

optional<size_t> TablaHash::buscar(const string&) const {
    throw logic_error("Pendiente: busqueda en la tabla hash propia.");
}

bool TablaHash::eliminar(const string&) {
    throw logic_error("Pendiente: eliminacion en la tabla hash propia.");
}
