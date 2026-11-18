#include "LectorCSV.h"

#include <stdexcept>

using namespace std;

LectorCSV::LectorCSV(const string& ruta) : ruta_(ruta) {}

bool LectorCSV::abrir() {
    if (estaAbierto()) {
        return true;
    }
    archivo_.clear();
    archivo_.open(ruta_);
    return archivo_.is_open();
}

void LectorCSV::cerrar() {
    if (estaAbierto()) {
        archivo_.close();
    }
}

bool LectorCSV::estaAbierto() const {
    return archivo_.is_open();
}

const string& LectorCSV::ruta() const {
    return ruta_;
}

optional<Cancion> LectorCSV::leerSiguiente() {
    throw logic_error("Pendiente: lectura incremental y conversion segura del CSV.");
}
