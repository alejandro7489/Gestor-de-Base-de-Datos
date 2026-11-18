#include "GestorMusical.h"

#include <stdexcept>

using namespace std;

GestorMusical::GestorMusical(const string& rutaCSV, const string& rutaBinario,
                           size_t capacidadBuffer, size_t tamanoPagina)
    : lectorCSV_(rutaCSV), almacenamiento_(rutaBinario, tamanoPagina),
      bufferPool_(capacidadBuffer, tamanoPagina) {}

bool GestorMusical::abrirCSV() {
    return lectorCSV_.abrir();
}

bool GestorMusical::abrirAlmacenamiento() {
    return almacenamiento_.abrir();
}

EstadisticasBuffer GestorMusical::estadisticasBuffer() const {
    return bufferPool_.estadisticas();
}

void GestorMusical::importarCSV() {
    throw logic_error("Pendiente: importacion incremental del CSV a paginas binarias.");
}

optional<Cancion> GestorMusical::buscarPorId(const string&) {
    throw logic_error("Pendiente: busqueda de canciones por track_id.");
}

vector<Cancion> GestorMusical::buscarPorArtista(const string&) {
    throw logic_error("Pendiente: busqueda de canciones por artista.");
}

vector<Cancion> GestorMusical::buscarPorAlbum(const string&) {
    throw logic_error("Pendiente: consulta de canciones por album.");
}

vector<Cancion> GestorMusical::filtrarPorAnio(int) {
    throw logic_error("Pendiente: filtro de canciones por anio.");
}

vector<Cancion> GestorMusical::filtrarPorGenero(const string&) {
    throw logic_error("Pendiente: filtro de canciones por genero.");
}

vector<Cancion> GestorMusical::rankingPopularidad(size_t) {
    throw logic_error("Pendiente: ranking de popularidad.");
}

void GestorMusical::insertar(const Cancion&) {
    throw logic_error("Pendiente: insercion de canciones.");
}

bool GestorMusical::actualizar(const string&, const Cancion&) {
    throw logic_error("Pendiente: actualizacion de canciones.");
}

bool GestorMusical::eliminar(const string&) {
    throw logic_error("Pendiente: eliminacion de canciones.");
}
