#ifndef SPOTIFY_DBMS_LECTOR_CSV_H
#define SPOTIFY_DBMS_LECTOR_CSV_H

#include "Cancion.h"

#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

using namespace std;

class LectorCSV {
public:
    explicit LectorCSV(const string& ruta);
    bool abrir();
    void cerrar();
    bool estaAbierto() const;
    const string& ruta() const;

    // Devuelve una canción por llamada; nullopt indica fin del archivo.
    optional<Cancion> leerSiguiente();

    inline static constexpr array<string_view, 16> COLUMNAS_ESPERADAS = {
        "name", "track_artists", "album_name", "album_release_date",
        "duration_ms", "genres", "explicit", "track_id", "track_track_number",
        "album_total_tracks", "streams", "artist_followers", "artist_popularity",
        "track_album_album", "popularity", "tempo"
    };

private:
    string ruta_;
    ifstream archivo_;
    size_t numeroRegistro_ = 0;
    bool encabezadoLeido_ = false;
};

#endif
