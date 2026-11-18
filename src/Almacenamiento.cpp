#include "Almacenamiento.h"

#include <filesystem>
#include <stdexcept>

using namespace std;

Almacenamiento::Almacenamiento(const string& ruta, size_t tamanoPagina)
    : ruta_(ruta), tamanoPagina_(tamanoPagina) {
    if (tamanoPagina_ == 0) {
        throw invalid_argument("El tamano de pagina debe ser mayor que cero.");
    }
}

bool Almacenamiento::abrir() {
    if (estaAbierto()) {
        return true;
    }
    archivo_.clear();
    archivo_.open(ruta_, ios::in | ios::out | ios::binary);
    return archivo_.is_open();
}

void Almacenamiento::cerrar() {
    if (estaAbierto()) {
        archivo_.close();
    }
}

bool Almacenamiento::estaAbierto() const {
    return archivo_.is_open();
}

const string& Almacenamiento::ruta() const {
    return ruta_;
}

size_t Almacenamiento::tamanoPagina() const {
    return tamanoPagina_;
}

uintmax_t Almacenamiento::tamanoArchivo() const {
    if (!estaAbierto()) {
        throw runtime_error("Debe abrir el almacenamiento antes de consultar su tamano.");
    }
    return filesystem::file_size(ruta_);
}

Pagina Almacenamiento::leerPagina(IdPagina) {
    throw logic_error("Pendiente: lectura de paginas del archivo binario.");
}

void Almacenamiento::escribirPagina(const Pagina&) {
    throw logic_error("Pendiente: escritura de paginas del archivo binario.");
}
