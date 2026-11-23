#include "GestorMusical.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace std;

namespace {
void comprobar(bool condicion, const string& mensaje) {
    if (!condicion) {
        throw runtime_error(mensaje);
    }
}

struct ArchivosDePrueba {
    filesystem::path carpeta;
    filesystem::path binario;
    filesystem::path csvDuplicados;
    filesystem::path binarioDuplicados;

    ArchivosDePrueba()
        : carpeta(filesystem::temp_directory_path() /
                  ("spotify-monday-" + to_string(chrono::steady_clock::now()
                                                     .time_since_epoch().count()))),
          binario(carpeta / "canciones.bin"),
          csvDuplicados(carpeta / "duplicados.csv"),
          binarioDuplicados(carpeta / "duplicados.bin") {
        comprobar(filesystem::create_directory(carpeta),
                  "La carpeta temporal de prueba ya existe.");
    }

    ~ArchivosDePrueba() {
        error_code error;
        filesystem::remove(binario, error);
        filesystem::remove(csvDuplicados, error);
        filesystem::remove(binarioDuplicados, error);
        filesystem::remove(carpeta, error);
    }
};

auto campos(const Cancion& c) {
    return tie(c.name, c.track_artists, c.album_name, c.album_release_date,
               c.duration_ms, c.genres, c.es_explicita, c.track_id,
               c.track_track_number, c.album_total_tracks, c.streams,
               c.artist_followers, c.artist_popularity, c.track_album_album,
               c.popularity, c.tempo);
}

void comprobarCancion(const optional<Cancion>& leida, const Cancion& esperada) {
    comprobar(leida && campos(*leida) == campos(esperada),
              "Alguno de los 16 campos difiere para track_id " + esperada.track_id);
}

void comprobarEstadisticas(EstadisticasBuffer stats, size_t hits, size_t misses,
                           size_t lecturas, const string& etapa) {
    comprobar(stats.hits == hits && stats.misses == misses && stats.lecturasDisco == lecturas,
              "Contadores incorrectos en " + etapa + " (hits=" + to_string(stats.hits)
              + ", misses=" + to_string(stats.misses)
              + ", lecturas=" + to_string(stats.lecturasDisco) + ").");
}

void probarBuffer(Almacenamiento& disco) {
    const size_t tamanoPagina = disco.tamanoPagina();
    comprobar(disco.cantidadPaginas() >= 3, "La prueba de buffer necesita tres páginas.");
    const Pagina paginaCero = disco.leerPagina(0);

    BufferPool secuencia(2, tamanoPagina);
    for (IdPagina id : {0, 0, 1, 2, 0}) {
        comprobar(secuencia.obtenerPagina(id, disco).id == id, "Se recuperó otra página.");
        secuencia.liberarPagina(id);
    }
    comprobarEstadisticas(secuencia.estadisticas(), 1, 4, 4, "secuencia 0,0,1,2,0");
    comprobar(secuencia.estaResidente(0) && secuencia.estaResidente(1)
              && !secuencia.estaResidente(2), "La víctima no fue el primer frame elegible.");

    BufferPool fijadas(2, tamanoPagina);
    const auto& cero = fijadas.obtenerPagina(0, disco);
    comprobar(&fijadas.obtenerPagina(0, disco) == &cero, "El hit no reutilizó el frame.");
    const auto& uno = fijadas.obtenerPagina(1, disco);
    fijadas.liberarPagina(0); // La página 0 todavía tiene un pin.
    bool sinFrame = false;
    try {
        (void)fijadas.obtenerPagina(2, disco);
    } catch (const runtime_error&) {
        sinFrame = true;
    }
    comprobar(sinFrame && cero.id == 0 && uno.id == 1 && cero.datos == paginaCero.datos
              && !fijadas.estaResidente(2), "Se reemplazó una página con pin activo.");
    comprobarEstadisticas(fijadas.estadisticas(), 1, 2, 2, "solicitud con dos frames fijados");
    fijadas.liberarPagina(1);
    comprobar(fijadas.obtenerPagina(2, disco).id == 2 && cero.id == 0
              && !fijadas.estaResidente(1), "Debía reemplazarse solo la página liberada.");
    fijadas.liberarPagina(2);
    fijadas.liberarPagina(0);

    // Con el archivo cerrado, el hit solo puede devolver los bytes que siguen en RAM.
    disco.cerrar();
    const auto& residente = fijadas.obtenerPagina(0, disco);
    comprobar(residente.datos == paginaCero.datos, "El hit no conservó los bytes del binario.");
    fijadas.liberarPagina(0);
    bool falloLectura = false;
    try {
        (void)fijadas.obtenerPagina(1, disco);
    } catch (const logic_error&) {
        falloLectura = true;
    }
    comprobar(falloLectura && fijadas.estaResidente(0) && fijadas.estaResidente(2),
              "Un fallo de lectura alteró las páginas residentes.");
    comprobarEstadisticas(fijadas.estadisticas(), 2, 3, 3, "hit y fallo con archivo cerrado");
    cout << "Buffer (0,0,1,2,0): 1 HIT, 4 MISS, 4 lecturas completas de página.\n"
         << "Pins: dos solicitudes a la misma página requieren dos liberaciones. "
            "HIT con archivo cerrado verificado.\n";
}

void probarLiberacionTrasError(const string& rutaBinario, const string& primerId) {
    GestorMusical gestor("", rutaBinario, 1);
    comprobar(gestor.abrirAlmacenamiento(), "No se pudo preparar la prueba de decodificación.");
    Almacenamiento disco(rutaBinario);
    comprobar(disco.abrir(), "No se pudo abrir el binario temporal para la prueba de error.");
    const Cancion siguiente = disco.leerPagina(1).leerCanciones().front();

    // Simular corrupción en el binario temporal después de indexarlo.
    Pagina corrupta(0);
    corrupta.datos[0] = byte{1}; // Declara un registro sin contenido.
    disco.escribirPagina(corrupta);
    disco.cerrar();

    bool errorDeRegistro = false;
    try {
        (void)gestor.buscarPorId(primerId);
    } catch (const runtime_error&) {
        errorDeRegistro = true;
    }
    comprobar(errorDeRegistro, "La consulta debía fallar al decodificar la página.");
    comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 1, 1, "página corrupta leída");
    comprobarCancion(gestor.buscarPorId(siguiente.track_id), siguiente);
    comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 2, 2, "reemplazo tras error de decodificación");
    cout << "Pin liberado tras un error de decodificación, verificado con un solo frame.\n";
}

void escribirCSVDePrueba(const string& ruta, const string& ultimoRegistro) {
    ofstream csv(ruta, ios::binary);
    const auto& columnas = LectorCSV::COLUMNAS_ESPERADAS;
    csv << columnas.front();
    for (size_t i = 1; i < columnas.size(); ++i) {
        csv << ',' << columnas[i];
    }
    csv << "\nTema Único " << string(150, 'a') << ",Ártista Norte,Disco Azul,,,,,id-primero,,,,,,,,\n"
        << "Otro Tema " << string(150, 'b') << ",Ártista Norte,Disco Verde,,,,,id-segundo,,,,,,,,\n"
        << ultimoRegistro;
    csv.close();
    comprobar(static_cast<bool>(csv), "No se pudo escribir el CSV auxiliar.");
}

void probarDuplicadosYError(const ArchivosDePrueba& archivos) {
    const string rutaCSV = archivos.csvDuplicados.string();
    const string rutaBinario = archivos.binarioDuplicados.string();
    constexpr size_t tamanoPagina = 256;

    escribirCSVDePrueba(rutaCSV, string(300, 'x') + ",,,,,,,id-no-cabe,,,,,,,,\n");
    GestorMusical importador(rutaCSV, rutaBinario, 1, tamanoPagina);
    bool rechazo = false;
    try {
        importador.importarCSV();
    } catch (const runtime_error& error) {
        rechazo = string(error.what()).find("id-no-cabe") != string::npos;
    }
    comprobar(rechazo && !filesystem::exists(rutaBinario) && !importador.buscarPorId("id-primero"),
              "La importación fallida dejó un binario o índice incompleto.");

    // Se repite sobre el mismo gestor y la misma ruta tras corregir la entrada de prueba.
    escribirCSVDePrueba(rutaCSV, "Duplicado,,,,,,,id-primero,,,,,,,,\n");
    importador.importarCSV();
    const auto primera = importador.buscarPorId("id-primero");
    comprobar(primera && primera->name == "Tema Único " + string(150, 'a'),
              "Se perdió la primera aparición.");
    bool existe = false;
    try {
        importador.importarCSV();
    } catch (const runtime_error&) {
        existe = true;
    }
    comprobar(existe, "Una nueva importación no debe sobrescribir el binario existente.");

    Almacenamiento disco(rutaBinario, tamanoPagina);
    comprobar(disco.abrir() && disco.cantidadPaginas() == 2
              && disco.leerPagina(0).leerCanciones().size() == 1
              && disco.leerPagina(1).leerCanciones().size() == 1,
              "El duplicado se escribió o se alteraron las páginas existentes.");
    Cancion duplicada = *primera;
    duplicada.name = "Duplicado de un binario anterior";
    Pagina pagina(2, tamanoPagina);
    comprobar(pagina.agregarCancion(duplicada), "No cupo el duplicado de prueba.");
    disco.escribirPagina(pagina);
    disco.cerrar();

    comprobar(importador.abrirAlmacenamiento(), "No se pudo reabrir el binario.");
    comprobarEstadisticas(importador.estadisticasBuffer(), 0, 0, 0, "reapertura del mismo gestor");
    comprobarCancion(importador.buscarPorId("id-primero"), *primera);
    comprobarEstadisticas(importador.estadisticasBuffer(), 0, 1, 1, "consulta por ID tras reabrir");
    const auto porNombre = importador.buscarPorNombre("tema único");
    comprobarEstadisticas(importador.estadisticasBuffer(), 1, 3, 3, "búsqueda por nombre");
    const auto porArtista = importador.buscarPorArtista("ÁRTISTA");
    comprobarEstadisticas(importador.estadisticasBuffer(), 1, 6, 6, "búsqueda por artista");
    const auto porAlbum = importador.buscarPorAlbum("verde");
    comprobarEstadisticas(importador.estadisticasBuffer(), 1, 9, 9, "búsqueda por álbum");
    comprobar(porNombre.size() == 1 && porNombre[0].track_id == "id-primero",
              "La búsqueda parcial por nombre falló.");
    comprobar(porArtista.size() == 2 && porArtista[0].track_id == "id-primero"
              && porArtista[1].track_id == "id-segundo", "La búsqueda por artista falló.");
    comprobar(porAlbum.size() == 1 && porAlbum[0].track_id == "id-segundo",
              "La búsqueda por álbum falló.");
    comprobar(importador.buscarPorNombre("").empty(), "El criterio vacío debe devolver cero coincidencias.");
    comprobarEstadisticas(importador.estadisticasBuffer(), 1, 9, 9, "criterio vacío");
    comprobar(importador.buscarPorAlbum("no-existe").empty(), "El álbum ausente debe devolver cero coincidencias.");
    comprobarEstadisticas(importador.estadisticasBuffer(), 1, 12, 12, "búsqueda sin coincidencias");
    cout << "Búsquedas por nombre, artista y álbum verificadas con el Buffer Pool.\n";
    cout << "Duplicados, protección del binario y reintento tras un registro demasiado grande: correctos.\n";
}

void probarIntegracion(const string& rutaCSV) {
    constexpr size_t totalEsperado = 1000;
    constexpr size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO;
    constexpr size_t cantidadFrames = 2;
    const ArchivosDePrueba archivos;
    const string rutaBinario = archivos.binario.string();

    {
        GestorMusical gestor(rutaCSV, rutaBinario, cantidadFrames, tamanoPagina);
        gestor.importarCSV();
        comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 0, 0, "importación");
    } // Cierra el archivo escrito antes de volver a abrirlo.

    LectorCSV lector(rutaCSV);
    comprobar(lector.abrir(), "No se pudo reabrir el CSV para comparar.");
    const auto esperada = lector.leerSiguiente();
    comprobar(esperada && !esperada->track_id.empty(), "El CSV no tiene primer ID.");

    GestorMusical gestor((archivos.carpeta / "sin.csv").string(), rutaBinario,
                         cantidadFrames, tamanoPagina);
    comprobar(gestor.abrirAlmacenamiento(), "No se pudo reabrir el binario importado.");
    const auto framesFrios = gestor.estadoFramesBuffer();
    comprobar(framesFrios.size() == 2 && !framesFrios[0].ocupado && !framesFrios[1].ocupado,
              "El monitor debe distinguir los dos frames libres al reabrir.");
    comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 0, 0, "reconstrucción del índice");

    comprobarCancion(gestor.buscarPorId(esperada->track_id), *esperada);
    comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 1, 1, "primera consulta");
    const auto frames = gestor.estadoFramesBuffer();
    comprobar(frames[0].ocupado && frames[0].pagina.id == 0 && frames[0].pinCount == 0
              && !frames[1].ocupado, "El monitor debe mostrar la página y los pines reales.");
    const auto residentes = frames[0].pagina.leerCanciones();
    comprobar(!residentes.empty() && campos(residentes.front()) == campos(*esperada),
              "El monitor debe mostrar los tracks de la página residente.");
    comprobarEstadisticas(gestor.estadisticasBuffer(), 0, 1, 1, "inspección de tracks residentes");
    comprobarCancion(gestor.buscarPorId(esperada->track_id), *esperada);
    comprobarEstadisticas(gestor.estadisticasBuffer(), 1, 1, 1, "consulta repetida");
    comprobar(!gestor.buscarPorId("#id-inexistente-prueba-monday#"), "Se encontró el ID inexistente.");
    comprobarEstadisticas(gestor.estadisticasBuffer(), 1, 1, 1, "consulta inexistente");

    Almacenamiento disco(rutaBinario, tamanoPagina);
    comprobar(disco.abrir(), "No se pudo abrir el binario para revisar sus páginas.");
    const size_t paginas = disco.cantidadPaginas();
    comprobar(disco.tamanoArchivo() == 8 + paginas * tamanoPagina, "Tamaño binario incorrecto.");

    size_t registros = 0;
    auto original = esperada;
    for (IdPagina id = 0; id < paginas; ++id) {
        const auto canciones = disco.leerPagina(id).leerCanciones();
        comprobar(!canciones.empty(), "Se escribió una página vacía.");
        for (const Cancion& cancion : canciones) {
            comprobar(original && campos(cancion) == campos(*original),
                      "Registro inesperado en página " + to_string(id));
            comprobarCancion(gestor.buscarPorId(original->track_id), *original);
            ++registros;
            original = lector.leerSiguiente();
        }
    }
    comprobar(!original && registros == totalEsperado, "El CSV y el binario deben tener 1000 canciones.");
    const auto recorrido = gestor.estadisticasBuffer();

    cout << "Importadas, reabiertas y verificadas por Hash en los 16 campos: " << registros << '\n'
         << "Páginas: " << paginas << " de " << tamanoPagina << " bytes. Binario: "
         << disco.tamanoArchivo() << " bytes.\n"
         << "Primera consulta: MISS y 1 lectura. Repetición: HIT y 0 lecturas adicionales. "
            "ID inexistente: nullopt y contadores intactos.\n"
         << "Buffer de consultas, incluyendo recorrido completo: " << recorrido.hits << " HIT, "
         << recorrido.misses << " MISS, " << recorrido.lecturasDisco << " lecturas de página.\n"
         << "Reconstrucción: " << paginas << " lecturas directas, fuera del contador del Buffer Pool.\n"
         << "Una lectura es una llamada exitosa a Almacenamiento::leerPagina; "
            "el contador no mide accesos físicos al dispositivo.\n";
    probarBuffer(disco);
    probarDuplicadosYError(archivos);
    probarLiberacionTrasError(rutaBinario, esperada->track_id);
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
