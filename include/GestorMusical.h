#ifndef SPOTIFY_DBMS_GESTOR_MUSICAL_H
#define SPOTIFY_DBMS_GESTOR_MUSICAL_H

#include "Almacenamiento.h"
#include "BufferPool.h"
#include "Cancion.h"
#include "LectorCSV.h"
#include "TablaHash.h"

#include <optional>
#include <string>
#include <vector>

using namespace std;

class GestorMusical {
public:
    explicit GestorMusical(
        const string& rutaCSV = "../spotify_reduced.csv",
        const string& rutaBinario = "data/canciones.bin",
        size_t capacidadBuffer = 16,
        size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO);

    bool abrirCSV();
    bool abrirAlmacenamiento();
    EstadisticasBuffer estadisticasBuffer() const;
    vector<Frame> estadoFramesBuffer() const;

    void importarCSV();
    optional<Cancion> buscarPorId(const string& trackId);
    vector<Cancion> buscarPorNombre(const string& nombre);
    vector<Cancion> buscarPorArtista(const string& artista);
    vector<Cancion> buscarPorAlbum(const string& album);

    // Consultas y operaciones pendientes.
    vector<Cancion> filtrarPorAnio(int anio);
    vector<Cancion> filtrarPorGenero(const string& genero);
    vector<Cancion> rankingPopularidad(size_t limite);
    void insertar(const Cancion& cancion);
    bool actualizar(const string& trackId, const Cancion& cancion);
    bool eliminar(const string& trackId);

private:
    enum class CampoBusqueda { Nombre, Artista, Album };
    vector<Cancion> buscarPorTexto(const string& texto, CampoBusqueda campo);
    vector<Cancion> leerCancionesConBuffer(IdPagina id);

    LectorCSV lectorCSV_;
    Almacenamiento almacenamiento_;
    TablaHash indiceCanciones_;
    BufferPool bufferPool_;
};

#endif
