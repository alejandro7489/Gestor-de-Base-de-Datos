#include "Pagina.h"

#include <cstring>
#include <limits>
#include <stdexcept>

using namespace std;

namespace {
// Agrega un entero a la salida, un byte a la vez, en orden little-endian.
// Así el archivo mantiene el mismo formato en equipos distintos.
void escribirU32(vector<byte>& salida, uint32_t valor) {
    // Un uint32 ocupa 4 bytes. Se agrega primero el byte de menor valor.
    for (int i = 0; i < 4; ++i) {
        salida.push_back(static_cast<byte>((valor >> (8 * i)) & 0xFF));
    }
}

void escribirU64(vector<byte>& salida, uint64_t valor) {
    // Un uint64 ocupa 8 bytes y usa la misma regla little-endian.
    for (int i = 0; i < 8; ++i) {
        salida.push_back(static_cast<byte>((valor >> (8 * i)) & 0xFF));
    }
}

void escribirTexto(vector<byte>& salida, const string& texto) {
    if (texto.size() > numeric_limits<uint32_t>::max()) {
        throw length_error("Texto demasiado largo para serializar.");
    }

    // Guardamos primero la longitud, para saber dónde termina el texto al leerlo.
    escribirU32(salida, static_cast<uint32_t>(texto.size()));
    for (unsigned char caracter : texto) {
        salida.push_back(static_cast<byte>(caracter));
    }
}

void escribirPresencia(vector<byte>& salida, bool presente) {
    // 0 significa vacío y 1 significa que después viene un valor.
    salida.push_back(presente ? byte{1} : byte{0});
}

void escribirInt(vector<byte>& salida, int valor) {
    escribirU32(salida, static_cast<uint32_t>(static_cast<int32_t>(valor)));
}

// Convierte Cancion a bytes siguiendo el orden de las columnas del CSV.
vector<byte> serializar(const Cancion& cancion) {
    vector<byte> bytes;

    // Los campos deben escribirse en el mismo orden en que se leerán luego.
    escribirTexto(bytes, cancion.name);
    escribirTexto(bytes, cancion.track_artists);
    escribirTexto(bytes, cancion.album_name);
    escribirTexto(bytes, cancion.album_release_date);

    escribirPresencia(bytes, cancion.duration_ms.has_value());
    if (cancion.duration_ms) escribirU64(bytes, *cancion.duration_ms);

    escribirTexto(bytes, cancion.genres);

    escribirPresencia(bytes, cancion.es_explicita.has_value());
    if (cancion.es_explicita) bytes.push_back(*cancion.es_explicita ? byte{1} : byte{0});

    escribirTexto(bytes, cancion.track_id);

    escribirPresencia(bytes, cancion.track_track_number.has_value());
    if (cancion.track_track_number) escribirU32(bytes, *cancion.track_track_number);

    escribirPresencia(bytes, cancion.album_total_tracks.has_value());
    if (cancion.album_total_tracks) escribirU32(bytes, *cancion.album_total_tracks);

    escribirPresencia(bytes, cancion.streams.has_value());
    if (cancion.streams) escribirU64(bytes, *cancion.streams);

    escribirPresencia(bytes, cancion.artist_followers.has_value());
    if (cancion.artist_followers) escribirU64(bytes, *cancion.artist_followers);

    escribirPresencia(bytes, cancion.artist_popularity.has_value());
    if (cancion.artist_popularity) escribirInt(bytes, *cancion.artist_popularity);

    escribirTexto(bytes, cancion.track_album_album);

    escribirPresencia(bytes, cancion.popularity.has_value());
    if (cancion.popularity) escribirInt(bytes, *cancion.popularity);

    escribirPresencia(bytes, cancion.tempo.has_value());
    if (cancion.tempo) {
        // double no se puede guardar como entero: copiamos sus 8 bytes tal cual.
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(*cancion.tempo));
        memcpy(&bits, &*cancion.tempo, sizeof(bits));
        escribirU64(bytes, bits);
    }

    return bytes;
}

class LectorBytes {
public:
    explicit LectorBytes(const vector<byte>& entrada) : bytes_(entrada) {}

    uint8_t leerByte() {
        // No leemos más allá del final del registro.
        exigir(1);
        return to_integer<uint8_t>(bytes_[posicion_++]);
    }

    uint32_t leerU32() {
        // Reconstruye un entero de 4 bytes, invirtiendo la escritura little-endian.
        uint32_t valor = 0;
        for (int i = 0; i < 4; ++i) {
            valor |= static_cast<uint32_t>(leerByte()) << (8 * i);
        }
        return valor;
    }

    uint64_t leerU64() {
        // Igual que leerU32, pero el número ocupa 8 bytes.
        uint64_t valor = 0;
        for (int i = 0; i < 8; ++i) {
            valor |= static_cast<uint64_t>(leerByte()) << (8 * i);
        }
        return valor;
    }

    string leerTexto() {
        const uint32_t longitud = leerU32();
        exigir(longitud);

        // La longitud dice cuántos bytes pertenecen al texto actual.
        string texto;
        texto.reserve(longitud);
        for (uint32_t i = 0; i < longitud; ++i) {
            texto.push_back(static_cast<char>(leerByte()));
        }
        return texto;
    }

    bool fin() const { return posicion_ == bytes_.size(); }

private:
    const vector<byte>& bytes_;
    size_t posicion_ = 0;

    void exigir(size_t cantidad) const {
        // Detecta archivos incompletos antes de acceder a una posición inválida.
        if (cantidad > bytes_.size() - posicion_) {
            throw runtime_error("Registro truncado en página.");
        }
    }
};

bool leerPresencia(LectorBytes& lector) {
    const uint8_t valor = lector.leerByte();
    // Un opcional solo puede guardar 0 o 1; otros valores indican datos corruptos.
    if (valor > 1) throw runtime_error("Indicador opcional inválido en página.");
    return valor == 1;
}

int leerInt(LectorBytes& lector) {
    return static_cast<int32_t>(lector.leerU32());
}

Cancion deserializar(const vector<byte>& bytes) {
    LectorBytes lector(bytes);
    Cancion cancion;

    // La lectura sigue exactamente el orden usado por serializar().
    cancion.name = lector.leerTexto();
    cancion.track_artists = lector.leerTexto();
    cancion.album_name = lector.leerTexto();
    cancion.album_release_date = lector.leerTexto();

    if (leerPresencia(lector)) cancion.duration_ms = lector.leerU64();
    cancion.genres = lector.leerTexto();

    if (leerPresencia(lector)) {
        const uint8_t valor = lector.leerByte();
        if (valor > 1) throw runtime_error("Valor booleano inválido en página.");
        cancion.es_explicita = (valor == 1);
    }

    cancion.track_id = lector.leerTexto();
    if (leerPresencia(lector)) cancion.track_track_number = lector.leerU32();
    if (leerPresencia(lector)) cancion.album_total_tracks = lector.leerU32();
    if (leerPresencia(lector)) cancion.streams = lector.leerU64();
    if (leerPresencia(lector)) cancion.artist_followers = lector.leerU64();
    if (leerPresencia(lector)) cancion.artist_popularity = leerInt(lector);

    cancion.track_album_album = lector.leerTexto();
    if (leerPresencia(lector)) cancion.popularity = leerInt(lector);

    if (leerPresencia(lector)) {
        const uint64_t bits = lector.leerU64();
        double tempo = 0;
        // Recuperamos el mismo patrón de bits que se guardó para tempo.
        memcpy(&tempo, &bits, sizeof(tempo));
        cancion.tempo = tempo;
    }

    if (!lector.fin()) throw runtime_error("El registro contiene bytes sobrantes.");
    return cancion;
}

uint32_t leerU32(const vector<byte>& datos, size_t posicion) {
    uint32_t valor = 0;
    for (int i = 0; i < 4; ++i) {
        valor |= static_cast<uint32_t>(to_integer<uint8_t>(datos[posicion + i])) << (8 * i);
    }
    return valor;
}

void ponerU32(vector<byte>& datos, size_t posicion, uint32_t valor) {
    for (int i = 0; i < 4; ++i) {
        datos[posicion + i] = static_cast<byte>((valor >> (8 * i)) & 0xFF);
    }
}
}

bool Pagina::agregarCancion(const Cancion& cancion) {
    if (datos.size() < 4 || datos.size() > numeric_limits<uint32_t>::max()) {
        throw runtime_error("Tamaño de página inválido.");
    }

    // Los primeros 4 bytes de la página guardan cuántas canciones contiene.
    const uint32_t cantidad = leerU32(datos, 0);
    size_t fin = 4;

    // Saltamos cada longitud y registro existente para encontrar el primer byte libre.
    for (uint32_t i = 0; i < cantidad; ++i) {
        if (fin + 4 > datos.size()) throw runtime_error("Página truncada.");
        const uint32_t longitud = leerU32(datos, fin);
        fin += 4;
        if (longitud > datos.size() - fin) throw runtime_error("Registro excede la página.");
        fin += longitud;
    }

    const vector<byte> registro = serializar(cancion);
    // Cada registro necesita 4 bytes para su longitud más su contenido.
    if (registro.size() > numeric_limits<uint32_t>::max()
        || fin + 4 + registro.size() > datos.size()) {
        return false;
    }

    ponerU32(datos, fin, static_cast<uint32_t>(registro.size()));
    fin += 4;
    for (byte valor : registro) datos[fin++] = valor;
    ponerU32(datos, 0, cantidad + 1);
    return true;
}

vector<Cancion> Pagina::leerCanciones() const {
    if (datos.size() < 4) throw runtime_error("Página demasiado pequeña.");

    // Usamos el contador inicial para saber cuántos registros decodificar.
    const uint32_t cantidad = leerU32(datos, 0);
    vector<Cancion> canciones;
    size_t posicion = 4;

    for (uint32_t i = 0; i < cantidad; ++i) {
        // La longitud permite copiar exactamente un registro antes de decodificarlo.
        if (posicion + 4 > datos.size()) throw runtime_error("Longitud de registro ausente.");
        const uint32_t longitud = leerU32(datos, posicion);
        posicion += 4;
        if (longitud > datos.size() - posicion) throw runtime_error("Registro truncado dentro de página.");

        vector<byte> registro(datos.begin() + static_cast<ptrdiff_t>(posicion),
                              datos.begin() + static_cast<ptrdiff_t>(posicion + longitud));
        canciones.push_back(deserializar(registro));
        posicion += longitud;
    }

    return canciones;
}
