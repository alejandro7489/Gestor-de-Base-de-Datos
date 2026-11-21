#include "GestorMusical.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {
void comprobar(bool condicion, const string& mensaje) {
    if (!condicion) {
        throw runtime_error(mensaje);
    }
}

// El test escribe en una carpeta nueva y limpia únicamente sus propios archivos.
struct ArchivosDePrueba {
    filesystem::path carpeta;
    filesystem::path binario;

    ArchivosDePrueba()
        : carpeta(filesystem::temp_directory_path() /
                  ("spotify-monday-" + to_string(chrono::steady_clock::now()
                                                     .time_since_epoch().count()))),
          binario(carpeta / "canciones.bin") {
        comprobar(filesystem::create_directory(carpeta),
                  "La carpeta temporal de prueba ya existe.");
    }

    ~ArchivosDePrueba() {
        error_code error;
        filesystem::remove(binario, error);
        filesystem::remove(carpeta, error);
    }
};

void probarIntegracion(const string& rutaCSV) {
    const size_t tamanoPagina = 2048;
    const size_t cantidadFrames = 2;
    const ArchivosDePrueba archivos;
    const string rutaBinario = archivos.binario.string();

    comprobar(filesystem::is_regular_file(rutaCSV), "No se encontró el CSV de prueba.");
    {
        GestorMusical gestor(rutaCSV, rutaBinario, cantidadFrames, tamanoPagina);
        gestor.importarCSV();
    } // Cierra el gestor que escribió el binario.

    LectorCSV lector(rutaCSV);
    comprobar(lector.abrir(), "No se pudo abrir el CSV para obtener el registro esperado.");
    const auto esperada = lector.leerSiguiente();
    comprobar(esperada.has_value() && !esperada->track_id.empty(),
              "El CSV debe empezar con una canción que tenga un ID.");
    lector.cerrar();

    // El segundo gestor tiene acceso al binario y recibe una ruta CSV inexistente.
    GestorMusical gestor((archivos.carpeta / "sin.csv").string(), rutaBinario,
                         cantidadFrames, tamanoPagina);
    comprobar(gestor.abrirAlmacenamiento(), "No se pudo reabrir el binario importado.");
    const auto inicial = gestor.estadisticasBuffer();
    comprobar(inicial.hits == 0 && inicial.misses == 0,
              "Reconstruir el índice debe dejar el buffer frío.");

    const auto primera = gestor.buscarPorId(esperada->track_id);
    comprobar(primera && primera->track_id == esperada->track_id
              && primera->name == esperada->name
              && primera->track_artists == esperada->track_artists
              && primera->album_name == esperada->album_name
              && primera->tempo == esperada->tempo,
              "La primera consulta debe recuperar la canción guardada en el binario.");
    const auto despuesMiss = gestor.estadisticasBuffer();
    comprobar(despuesMiss.hits == 0 && despuesMiss.misses == 1,
              "La primera consulta debe producir un MISS.");

    const auto segunda = gestor.buscarPorId(esperada->track_id);
    comprobar(segunda && segunda->track_id == primera->track_id && segunda->name == primera->name,
              "La segunda consulta debe recuperar la misma canción.");
    const auto despuesHit = gestor.estadisticasBuffer();
    comprobar(despuesHit.hits == 1 && despuesHit.misses == 1,
              "La segunda consulta debe producir un HIT sin otro MISS.");

    comprobar(!gestor.buscarPorId("#id-inexistente-prueba-monday#"),
              "La búsqueda de un ID ajeno al catálogo debe devolver nullopt.");
    const auto final = gestor.estadisticasBuffer();
    comprobar(final.hits == despuesHit.hits && final.misses == despuesHit.misses,
              "Consultar un ID inexistente debe conservar los contadores.");

    cout << "Pasó la prueba de integración. Canción recuperada: " << primera->name << '\n'
         << "Buffer: " << final.hits << " HIT, " << final.misses << " MISS.\n";
}
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: monday <ruta-del-CSV>\n";
        return 2;
    }

    try {
        probarIntegracion(argv[1]);
        return 0;
    } catch (const exception& error) {
        cerr << "Falló la prueba de integración: " << error.what() << '\n';
        return 1;
    }
}
