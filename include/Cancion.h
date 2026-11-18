#ifndef SPOTIFY_DBMS_CANCION_H
#define SPOTIFY_DBMS_CANCION_H

#include <cstdint>
#include <optional>
#include <string>

using namespace std;

struct Cancion {
    string name;
    string track_artists;
    string album_name;
    string album_release_date;
    optional<uint64_t> duration_ms;
    string genres;
    // La columna "explicit" no puede usarse como nombre: es una palabra de C++.
    optional<bool> es_explicita;
    string track_id;
    optional<uint32_t> track_track_number;
    optional<uint32_t> album_total_tracks;
    optional<uint64_t> streams;
    optional<uint64_t> artist_followers;
    optional<int> artist_popularity;
    string track_album_album;
    optional<int> popularity;
    optional<double> tempo;
};

#endif
