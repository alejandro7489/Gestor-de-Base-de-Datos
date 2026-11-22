#include "LectorCSV.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

using namespace std;

namespace {
bool leerRegistroCSV(istream& entrada, vector<string>& campos) {
    campos.clear();
    string campo;
    bool entreComillas = false;
    bool inicioCampo = true;
    bool leyoAlgo = false;
    char caracter;

    while (entrada.get(caracter)) {
        leyoAlgo = true;
        if (entreComillas) {
            // Dentro de comillas, una coma o salto de línea pertenece al texto.
            if (caracter == '"') {
                if (entrada.peek() == '"') {
                    entrada.get();
                    campo += '"';
                } else {
                    entreComillas = false;
                }
            } else {
                campo += caracter;
            }
        } else if (inicioCampo && caracter == '"') {
            entreComillas = true;
            inicioCampo = false;
        } else if (caracter == ',') {
            campos.push_back(campo);
            campo.clear();
            inicioCampo = true;
        } else if (caracter == '\n' || caracter == '\r') {
            if (caracter == '\r' && entrada.peek() == '\n') {
                entrada.get();
            }
            campos.push_back(campo);
            return true;
        } else {
            campo += caracter;
            inicioCampo = false;
        }
    }

    if (entreComillas) {
        throw runtime_error("Campo entrecomillado sin cierre.");
    }
    if (!leyoAlgo) {
        return false;
    }
    campos.push_back(campo);
    return true;
}

string limpiarBOM(string texto) {
    if (texto.size() >= 3
        && static_cast<unsigned char>(texto[0]) == 0xEF
        && static_cast<unsigned char>(texto[1]) == 0xBB
        && static_cast<unsigned char>(texto[2]) == 0xBF) {
        texto.erase(0, 3);
    }
    return texto;
}

template <typename T>
optional<T> convertirEntero(const string& texto, const string& columna) {
    if (texto.empty()) {
        return nullopt;
    }
    size_t fin = 0;
    long double valor = 0;
    try {
        valor = stold(texto, &fin);
    } catch (const exception&) {
        throw runtime_error("valor entero inválido en columna " + columna + ": " + texto);
    }
    // Rechazamos decimales y números fuera del rango del tipo destino.
    if (fin != texto.size() || !isfinite(static_cast<double>(valor))
        || floor(valor) != valor
        || valor < static_cast<long double>(numeric_limits<T>::lowest())
        || valor > static_cast<long double>(numeric_limits<T>::max())) {
        throw runtime_error("valor entero fuera de formato o rango en columna "
                            + columna + ": " + texto);
    }
    return static_cast<T>(valor);
}

optional<bool> convertirBool(const string& texto) {
    if (texto.empty()) return nullopt;
    if (texto == "0" || texto == "false" || texto == "False") return false;
    if (texto == "1" || texto == "true" || texto == "True") return true;
    throw runtime_error("valor booleano inválido en columna explicit: " + texto);
}

optional<double> convertirDouble(const string& texto) {
    if (texto.empty()) return nullopt;
    size_t fin = 0;
    double valor;
    try {
        valor = stod(texto, &fin);
    } catch (const exception&) {
        throw runtime_error("valor decimal inválido en columna tempo: " + texto);
    }
    if (fin != texto.size() || !isfinite(valor)) {
        throw runtime_error("valor decimal inválido en columna tempo: " + texto);
    }
    return valor;
}
}

LectorCSV::LectorCSV(const string& ruta) : ruta_(ruta) {}

bool LectorCSV::abrir() {
    if (estaAbierto()) return true;
    archivo_.clear();
    archivo_.open(ruta_, ios::in | ios::binary);
    if (!archivo_.is_open()) return false;
    numeroRegistro_ = 0;
    encabezadoLeido_ = false;
    return true;
}

void LectorCSV::cerrar() {
    if (estaAbierto()) archivo_.close();
}

bool LectorCSV::estaAbierto() const { return archivo_.is_open(); }
const string& LectorCSV::ruta() const { return ruta_; }

optional<Cancion> LectorCSV::leerSiguiente() {
    if (!estaAbierto()) throw logic_error("Debe abrir el CSV antes de leerlo.");
    try {
        vector<string> campos;
        if (!encabezadoLeido_) {
            if (!leerRegistroCSV(archivo_, campos)) {
                throw runtime_error("El archivo CSV está vacío; falta el encabezado.");
            }
            numeroRegistro_ = 1;
            if (!campos.empty()) campos[0] = limpiarBOM(campos[0]);
            if (campos.size() != COLUMNAS_ESPERADAS.size()) {
                throw runtime_error("El encabezado no contiene exactamente 16 columnas.");
            }
            for (size_t i = 0; i < campos.size(); ++i) {
                if (campos[i] != COLUMNAS_ESPERADAS[i]) {
                    throw runtime_error("Encabezado incompatible en la columna "
                                        + to_string(i + 1) + ": " + campos[i]);
                }
            }
            encabezadoLeido_ = true;
        }

        if (!leerRegistroCSV(archivo_, campos)) return nullopt;
        ++numeroRegistro_;
        if (campos.size() != COLUMNAS_ESPERADAS.size()) {
            throw runtime_error("se esperaban 16 columnas y se encontraron "
                                + to_string(campos.size()));
        }
        Cancion cancion;
        // Los índices corresponden al orden de columnas declarado en el contrato.
        cancion.name = campos[0];
        cancion.track_artists = campos[1];
        cancion.album_name = campos[2];
        cancion.album_release_date = campos[3];
        cancion.duration_ms = convertirEntero<uint64_t>(campos[4], "duration_ms");
        cancion.genres = campos[5];
        cancion.es_explicita = convertirBool(campos[6]);
        cancion.track_id = campos[7];
        cancion.track_track_number = convertirEntero<uint32_t>(campos[8], "track_track_number");
        cancion.album_total_tracks = convertirEntero<uint32_t>(campos[9], "album_total_tracks");
        cancion.streams = convertirEntero<uint64_t>(campos[10], "streams");
        cancion.artist_followers = convertirEntero<uint64_t>(campos[11], "artist_followers");
        cancion.artist_popularity = convertirEntero<int>(campos[12], "artist_popularity");
        cancion.track_album_album = campos[13];
        cancion.popularity = convertirEntero<int>(campos[14], "popularity");
        cancion.tempo = convertirDouble(campos[15]);
        return cancion;
    } catch (const exception& error) {
        throw runtime_error("Error en registro CSV " + to_string(numeroRegistro_)
                            + ": " + error.what());
    }
}
