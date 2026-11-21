#include "TablaHash.h"

#include <cstdint>
#include <stdexcept>

using namespace std;

TablaHash::TablaHash(size_t capacidad) : buckets_(capacidad, nullptr) {
    if (capacidad == 0) {
        throw invalid_argument("La capacidad de la tabla debe ser mayor que cero.");
    }
}

TablaHash::TablaHash(const TablaHash& otra) : TablaHash(otra.capacidad()) {
    // Cada tabla posee sus propios nodos, incluso cuando se copia.
    for (size_t i = 0; i < capacidad(); ++i) {
        for (const Nodo* nodo = otra.buckets_[i]; nodo != nullptr; nodo = nodo->siguiente) {
            buckets_[i] = new Nodo{nodo->clave, nodo->posicion, buckets_[i]};
        }
    }
}

TablaHash& TablaHash::operator=(const TablaHash& otra) {
    if (this != &otra) {
        TablaHash copia(otra);
        // Al salir, copia libera los nodos anteriores de esta tabla
        buckets_.swap(copia.buckets_);
    }
    return *this;
}

TablaHash::~TablaHash() {
    for (Nodo* primero : buckets_) {
        while (primero != nullptr) {
            Nodo* siguiente = primero->siguiente;
            delete primero;
            primero = siguiente;
        }
    }
}

size_t TablaHash::capacidad() const {
    return buckets_.size();
}

size_t TablaHash::indiceBucket(const string& clave) const {
    // Se mezcla cada byte en orden. El desbordamiento sin signo es parte del cálculo.
    uint64_t hash = 0;
    for (unsigned char byte : clave) {
        hash = hash * 31 + byte;
    }
    return hash % capacidad();
}

void TablaHash::insertar(const string& clave, size_t posicion) {
    Nodo*& primero = buckets_[indiceBucket(clave)];
    for (Nodo* nodo = primero; nodo != nullptr; nodo = nodo->siguiente) {
        if (nodo->clave == clave) {
            nodo->posicion = posicion;
            return;
        }
    }
    primero = new Nodo{clave, posicion, primero};
}

optional<size_t> TablaHash::buscar(const string& clave) const {
    for (const Nodo* nodo = buckets_[indiceBucket(clave)];
         nodo != nullptr; nodo = nodo->siguiente) {
        if (nodo->clave == clave) {
            return nodo->posicion;
        }
    }
    return nullopt;
}

bool TablaHash::eliminar(const string& clave) {
    Nodo*& primero = buckets_[indiceBucket(clave)];
    Nodo* anterior = nullptr;
    for (Nodo* nodo = primero; nodo != nullptr; nodo = nodo->siguiente) {
        if (nodo->clave == clave) {
            // Si es el primer nodo, cambia la cabeza del bucket
            if (anterior == nullptr) {
                primero = nodo->siguiente;
            } else {
                anterior->siguiente = nodo->siguiente;
            }
            delete nodo;
            return true;
        }
        anterior = nodo;
    }
    return false;
}
