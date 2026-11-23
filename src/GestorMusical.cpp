#include "GestorMusical.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {
string aMinusculas(string texto) {
    for (size_t i = 0; i < texto.size(); ++i) {
        const auto letra = static_cast<unsigned char>(texto[i]);
        if (letra >= 'A' && letra <= 'Z') {
            texto[i] = static_cast<char>(letra + ('a' - 'A'));
        } else if (letra == 0xC3 && i + 1 < texto.size()) {
            // En UTF-8, las mayúsculas latinas como Á, É y Ñ empiezan por C3.
            const auto segundoByte = static_cast<unsigned char>(texto[i + 1]);
            if (segundoByte >= 0x80 && segundoByte <= 0x9E && segundoByte != 0x97) {
                texto[i + 1] = static_cast<char>(segundoByte + 0x20);
            }
        }
    }
    return texto;
}
}

GestorMusical::GestorMusical(const string& rutaCSV, const string& rutaBinario,
                           size_t capacidadBuffer, size_t tamanoPagina)
    : lectorCSV_(rutaCSV), almacenamiento_(rutaBinario, tamanoPagina),
      bufferPool_(capacidadBuffer, tamanoPagina) {}

bool GestorMusical::abrirCSV() {
    return lectorCSV_.abrir();
}

bool GestorMusical::abrirAlmacenamiento() {
    Almacenamiento reabierto(almacenamiento_.ruta(), almacenamiento_.tamanoPagina());
    if (!reabierto.abrir()) {
        return false;
    }

    TablaHash indice(indiceCanciones_.capacidad());
    const size_t paginas = reabierto.cantidadPaginas();
    for (IdPagina id = 0; id < paginas; ++id) {
        // Leer directamente para que la primera consulta encuentre el buffer vacío.
        for (const Cancion& cancion : reabierto.leerPagina(id).leerCanciones()) {
            if (!indice.buscar(cancion.track_id)) {
                indice.insertar(cancion.track_id, id);
            }
        }
    }

    bufferPool_ = BufferPool(bufferPool_.capacidad(), reabierto.tamanoPagina());
    indiceCanciones_ = indice;
    almacenamiento_ = move(reabierto);
    return true;
}

EstadisticasBuffer GestorMusical::estadisticasBuffer() const {
    return bufferPool_.estadisticas();
}

void GestorMusical::importarCSV() {
    LectorCSV lector(lectorCSV_.ruta());
    if (!lector.abrir()) {
        throw runtime_error("No se pudo abrir el CSV " + lector.ruta());
    }

    Almacenamiento destino(almacenamiento_.ruta(), almacenamiento_.tamanoPagina());
    if (!destino.crear()) {
        throw runtime_error("No se pudo crear un binario nuevo en " + destino.ruta());
    }

    try {
        TablaHash indice(indiceCanciones_.capacidad());
        Pagina pagina(0, destino.tamanoPagina());
        bool paginaConCanciones = false;
        while (const auto cancion = lector.leerSiguiente()) {
            if (indice.buscar(cancion->track_id)) {
                continue;
            }

            if (!pagina.agregarCancion(*cancion)) {
                Pagina siguiente(pagina.id + 1, destino.tamanoPagina());
                if (!paginaConCanciones || !siguiente.agregarCancion(*cancion)) {
                    throw runtime_error("La cancion no cabe en una pagina vacia. track_id: "
                                        + cancion->track_id);
                }
                destino.escribirPagina(pagina);
                pagina = move(siguiente);
            }
            paginaConCanciones = true;
            indice.insertar(cancion->track_id, pagina.id);
        }
        if (paginaConCanciones) {
            destino.escribirPagina(pagina);
        }

        bufferPool_ = BufferPool(bufferPool_.capacidad(), destino.tamanoPagina());
        indiceCanciones_ = indice;
        almacenamiento_ = move(destino);
    } catch (...) {
        // crear() tuvo éxito, así que este archivo pertenece a esta importación.
        destino.cerrar();
        error_code error;
        filesystem::remove(destino.ruta(), error);
        if (error) {
            throw runtime_error("No se pudo retirar el binario incompleto: " + destino.ruta()
                                + " (" + error.message() + ")");
        }
        throw;
    }
}

optional<Cancion> GestorMusical::buscarPorId(const string& trackId) {
    const auto id = indiceCanciones_.buscar(trackId);
    if (!id) {
        return nullopt;
    }

    for (Cancion& cancion : leerCancionesConBuffer(*id)) {
        if (cancion.track_id == trackId) {
            return move(cancion);
        }
    }
    throw runtime_error("El indice apunta a una pagina sin el track_id " + trackId);
}

vector<Cancion> GestorMusical::leerCancionesConBuffer(IdPagina id) {
    const Pagina& pagina = bufferPool_.obtenerPagina(id, almacenamiento_);
    vector<Cancion> canciones;
    try {
        canciones = pagina.leerCanciones();
    } catch (...) {
        bufferPool_.liberarPagina(id);
        throw;
    }
    bufferPool_.liberarPagina(id);
    return canciones;
}

vector<Cancion> GestorMusical::buscarPorNombre(const string& nombre) {
    return buscarPorTexto(nombre, CampoBusqueda::Nombre);
}

vector<Cancion> GestorMusical::buscarPorArtista(const string& artista) {
    return buscarPorTexto(artista, CampoBusqueda::Artista);
}

vector<Cancion> GestorMusical::buscarPorAlbum(const string& album) {
    return buscarPorTexto(album, CampoBusqueda::Album);
}

vector<Cancion> GestorMusical::buscarPorTexto(const string& texto, CampoBusqueda campo) {
    vector<Cancion> resultados;
    if (texto.empty() || !almacenamiento_.estaAbierto()) {
        return resultados;
    }

    const string criterio = aMinusculas(texto);
    const size_t paginas = almacenamiento_.cantidadPaginas();
    for (IdPagina id = 0; id < paginas; ++id) {
        TablaHash idsEnPagina;
        for (Cancion& cancion : leerCancionesConBuffer(id)) {
            if (indiceCanciones_.buscar(cancion.track_id) != id || idsEnPagina.buscar(cancion.track_id)) {
                continue;
            }
            idsEnPagina.insertar(cancion.track_id, id);
            const string& valor = campo == CampoBusqueda::Nombre ? cancion.name
                                : campo == CampoBusqueda::Artista ? cancion.track_artists
                                : cancion.album_name;
            if (aMinusculas(valor).find(criterio) != string::npos) {
                resultados.push_back(move(cancion));
            }
        }
    }
    return resultados;
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
