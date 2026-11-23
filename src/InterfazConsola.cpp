#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include "InterfazConsola.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
// Utilidades internas (solo visibles en este archivo)
// ---------------------------------------------------------------------------
namespace {

// Colores ANSI
const string RESET    = "\033[0m";
const string NEGRITA  = "\033[1m";
const string VERDE    = "\033[38;5;41m";
const string GRIS     = "\033[90m";
const string AMARILLO = "\033[33m";
const string ROJO     = "\033[31m";

const string RUTA_CSV = "../datasets/spotify_sample_1000.csv";
const string RUTA_BINARIO = "data/canciones-demo.bin";
constexpr size_t CANTIDAD_FRAMES = 2;

const int ANCHO_PANTALLA = 62;

string repetir(const string& texto, int veces) {
    string resultado;
    for (int i = 0; i < veces; i++) resultado += texto;
    return resultado;
}

// Longitud visible: ignora códigos ANSI y cuenta cada carácter UTF-8 una vez.
size_t longitudVisible(const string& s) {
    size_t n = 0;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1B) {
            while (i < s.size() && s[i] != 'm') i++;
        } else if ((c & 0xC0) != 0x80) {
            n++;
        }
    }
    return n;
}

string rellenar(const string& s, size_t ancho) {
    size_t largo = longitudVisible(s);
    return largo >= ancho ? s : s + string(ancho - largo, ' ');
}

string recortar(const string& s) {
    size_t inicio = s.find_first_not_of(" \t\r\n");
    if (inicio == string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r\n");
    return s.substr(inicio, fin - inicio + 1);
}

string indicador(const string& contexto) {
    string ruta = contexto.empty() ? string("") : "/" + contexto;
    return "  " + VERDE + "spotify-db" + ruta + " > " + RESET;
}

// Barra de progreso: ████████░░░░
string barra(double porcentaje, int ancho) {
    int llenos = static_cast<int>(round(porcentaje / 100.0 * ancho));
    if (llenos < 0) llenos = 0;
    if (llenos > ancho) llenos = ancho;
    return VERDE + repetir("█", llenos) + GRIS + repetir("░", ancho - llenos) + RESET;
}

void imprimirCaja(const vector<string>& lineas, size_t ancho) {
    cout << GRIS << "┌" << repetir("─", static_cast<int>(ancho) + 2) << "┐" << RESET << "\n";
    for (size_t i = 0; i < lineas.size(); i++) {
        cout << GRIS << "│ " << RESET << rellenar(lineas[i], ancho) << GRIS << " │" << RESET << "\n";
    }
    cout << GRIS << "└" << repetir("─", static_cast<int>(ancho) + 2) << "┘" << RESET << "\n";
}

void imprimirTabla(const vector<string>& cabecera,
                   const vector<size_t>& anchos,
                   const vector<vector<string>>& filas) {
    cout << "  " << NEGRITA;
    for (size_t c = 0; c < cabecera.size(); c++) cout << rellenar(cabecera[c], anchos[c]) << "  ";
    cout << RESET << "\n  " << GRIS;
    for (size_t c = 0; c < cabecera.size(); c++) cout << repetir("─", static_cast<int>(anchos[c])) << "  ";
    cout << RESET << "\n";
    for (size_t f = 0; f < filas.size(); f++) {
        cout << "  ";
        for (size_t c = 0; c < filas[f].size(); c++) cout << rellenar(filas[f][c], anchos[c]) << "  ";
        cout << "\n";
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Bucle principal
// ---------------------------------------------------------------------------
void InterfazConsola::ejecutar() {
    prepararConsola();
    try {
        reabrirBase();
    } catch (const exception& error) {
        cout << "  Error al abrir el binario: " << error.what() << '\n';
        pausar();
    }

    bool salir = false;
    while (!salir) {
        limpiarPantalla();
        mostrarBanner();
        mostrarEstado();
        mostrarMenuPrincipal();

        string opcion = leerOpcion("");

        if      (opcion == "1") pantallaBuscarMusica();
        else if (opcion == "2") pantallaTop();
        else if (opcion == "3") pantallaExplorar();
        else if (opcion == "4") pantallaOperaciones();
        else if (opcion == "5") pantallaBufferPool();
        else if (opcion == "6") pantallaEstadisticas();
        else if (opcion == "0") salir = true;
        else                    mostrarOpcionInvalida();
    }

    cout << "\n  " << VERDE << "Hasta pronto." << RESET << "\n\n";
}

// ---------------------------------------------------------------------------
// Utilidades de presentación
// ---------------------------------------------------------------------------
void InterfazConsola::prepararConsola() const {
#ifdef _WIN32
    // UTF-8 y secuencias ANSI (colores) en la consola de Windows.
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    HANDLE salida = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    if (salida != INVALID_HANDLE_VALUE && GetConsoleMode(salida, &modo)) {
        SetConsoleMode(salida, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void InterfazConsola::limpiarPantalla() const {
    cout << "\033[2J\033[H";
}

void InterfazConsola::mostrarBanner() const {
    const string banner = R"BANNER(
 ____   ____    ___   _____  ___  _____ __   __
/ ___| |  _ \  / _ \ |_   _||_ _||  ___|\ \ / /
\___ \ | |_) || | | |  | |   | | | |_    \ V / 
 ___) ||  __/ | |_| |  | |   | | |  _|    | |  
|____/ |_|     \___/   |_|  |___||_|      |_|  
)BANNER";

    cout << VERDE << banner << RESET << "\n";
    cout << GRIS << repetir("─", ANCHO_PANTALLA) << RESET << "\n\n";
    cout << NEGRITA << "SPOTIFY DATABASE MANAGER" << RESET << "\n";
    cout << GRIS << "Proyecto de Estructura de Datos · C++17" << RESET << "\n\n";
}

void InterfazConsola::mostrarEstado() const {
    vector<string> lineas;
    if (gestor_) {
        lineas.push_back(VERDE + NEGRITA + "● BASE DE DATOS EN LÍNEA" + RESET);
        lineas.push_back("Canciones: " + to_string(canciones_) + " · Páginas: " + to_string(paginas_));
    } else {
        lineas.push_back(AMARILLO + "● BASE DE DATOS SIN ABRIR" + RESET);
        lineas.push_back("Importa el CSV desde Operaciones [4].");
    }
    lineas.push_back("Buffer: " + to_string(CANTIDAD_FRAMES) + " frames · Página: "
                     + to_string(TAMANO_PAGINA_PREDETERMINADO) + " bytes");
    imprimirCaja(lineas, 46);
}

void InterfazConsola::mostrarMenuPrincipal() const {
    const char* opciones[] = {
        "Buscar música",
        "Top de popularidad",
        "Explorar biblioteca",
        "Operaciones de base de datos",
        "Monitor del Buffer Pool",
        "Estadísticas de rendimiento"
    };

    cout << "\n";
    for (int i = 0; i < 6; i++) {
        cout << "  " << VERDE << "[" << (i + 1) << "]" << RESET << "  " << opciones[i] << "\n";
    }
    cout << "\n  " << VERDE << "[0]" << RESET << "  Salir\n\n";
    cout << GRIS << repetir("─", ANCHO_PANTALLA) << RESET << "\n";
}

void InterfazConsola::mostrarEncabezado(const string& titulo) const {
    limpiarPantalla();
    cout << "\n  " << VERDE << NEGRITA << "SPOTIFY DATABASE MANAGER" << RESET
         << GRIS << "  ›  " << RESET << NEGRITA << titulo << RESET << "\n";
    cout << GRIS << repetir("─", ANCHO_PANTALLA) << RESET << "\n\n";
}

void InterfazConsola::mostrarPendiente(const string& detalle) const {
    cout << "\n  " << AMARILLO << "Pendiente: " << RESET << detalle << "\n";
}

void InterfazConsola::mostrarOpcionInvalida() const {
    cout << "\n  " << ROJO << "Opción no válida." << RESET << "\n";
    pausar();
}

void InterfazConsola::pausar() const {
    leerLinea("\n  " + GRIS + "Presiona Enter para continuar..." + RESET);
}

string InterfazConsola::leerLinea(const string& indicadorTexto) const {
    cout << indicadorTexto << flush;
    string linea;
    if (!getline(cin, linea)) {
        // Fin de entrada (Ctrl+Z / Ctrl+D): se trata como "0" para salir o volver.
        return "0";
    }
    return linea;
}

string InterfazConsola::leerOpcion(const string& contexto) const {
    return recortar(leerLinea(indicador(contexto)));
}

// ---------------------------------------------------------------------------
// [1] Buscar música
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaBuscarMusica() {
    while (true) {
        mostrarEncabezado("Buscar música");
        cout << "  " << VERDE << "[1]" << RESET << "  Por ID de canción (track_id)\n"
             << "  " << VERDE << "[2]" << RESET << "  Por nombre de canción\n"
             << "  " << VERDE << "[3]" << RESET << "  Por artista\n"
             << "  " << VERDE << "[4]" << RESET << "  Por álbum\n"
             << "  " << VERDE << "[5]" << RESET << "  Filtrar por año o género\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        const string opcion = leerOpcion("buscar");
        if (opcion == "0") return;
        if (opcion.size() != 1 || opcion[0] < '1' || opcion[0] > '5') {
            mostrarOpcionInvalida();
            continue;
        }
        if (opcion == "5") {
            mostrarPendiente("filtros por año y género.");
        } else if (!gestor_) {
            cout << "\n  Importa el CSV desde Operaciones [4].\n";
        } else {
            try {
                if (opcion == "1") {
                    buscarIdEnConsola();
                } else {
                    buscarTextoEnConsola(opcion);
                }
            } catch (const exception& error) {
                cout << "\n  Error en la consulta: " << error.what() << '\n';
            }
        }
        pausar();
    }
}

void InterfazConsola::buscarIdEnConsola() {
    if (primerId_) {
        cout << "\n  ID para probar: " << *primerId_ << " (presiona Enter para usarlo)\n";
    }
    string id = recortar(leerLinea("  track_id: "));
    if (!cin) return;
    if (id.empty() && primerId_) id = *primerId_;
    if (id.empty()) {
        cout << "\n  Escribe un track_id para buscar.\n";
        return;
    }

    const auto antes = gestor_->estadisticasBuffer();
    const auto cancion = gestor_->buscarPorId(id);
    const auto despues = gestor_->estadisticasBuffer();
    ++consultas_;
    if (!cancion) {
        ++idsAusentes_;
        cout << "\n  ID inexistente. El Buffer Pool conserva sus contadores.\n";
        return;
    }

    cout << "\n  " << NEGRITA << cancion->name << RESET << '\n'
         << "  Artista: " << cancion->track_artists << '\n'
         << "  Álbum:   " << cancion->album_name << '\n'
         << "  ID:      " << cancion->track_id << '\n'
         << "  Buffer:  " << (despues.hits > antes.hits ? "HIT" : "MISS")
         << " · Lecturas de página: "
         << despues.lecturasDisco - antes.lecturasDisco << '\n';
}

void InterfazConsola::buscarTextoEnConsola(const string& opcion) {
    const string campo = opcion == "2" ? "Nombre de la canción"
                       : opcion == "3" ? "Artista" : "Álbum";
    const string texto = recortar(leerLinea("\n  " + campo + " (puedes escribir solo una parte): "));
    if (!cin) return;
    if (texto.empty()) {
        cout << "\n  Escribe un texto para buscar.\n";
        return;
    }

    vector<Cancion> canciones;
    if (opcion == "2") {
        canciones = gestor_->buscarPorNombre(texto);
    } else if (opcion == "3") {
        canciones = gestor_->buscarPorArtista(texto);
    } else {
        canciones = gestor_->buscarPorAlbum(texto);
    }

    if (canciones.empty()) {
        cout << "\n  No se encontraron canciones.\n";
        return;
    }
    cout << "\n  Coincidencias: " << canciones.size() << "\n\n";
    for (const Cancion& cancion : canciones) {
        cout << "  " << NEGRITA << cancion.name << RESET << " · " << cancion.track_artists
             << " · " << cancion.album_name << "\n  ID: " << cancion.track_id << "\n\n";
    }
}

void InterfazConsola::pantallaTop() const {
    mostrarEncabezado("Top de popularidad");
    mostrarPendiente("ranking de popularidad.");
    pausar();
}

void InterfazConsola::pantallaExplorar() const {
    mostrarEncabezado("Explorar biblioteca");
    mostrarPendiente("exploración por artista, álbum, género y año.");
    pausar();
}

// ---------------------------------------------------------------------------
// [4] Operaciones de base de datos
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaOperaciones() {
    while (true) {
        mostrarEncabezado("Operaciones de base de datos");
        cout << "  " << VERDE << "[1]" << RESET << "  Insertar canción\n"
             << "  " << VERDE << "[2]" << RESET << "  Actualizar canción\n"
             << "  " << VERDE << "[3]" << RESET << "  Eliminar canción\n"
             << "  " << VERDE << "[4]" << RESET << "  Importar CSV\n"
             << "  " << VERDE << "[5]" << RESET << "  Reabrir binario y enfriar el buffer\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        const string opcion = leerOpcion("ops");
        if (opcion == "0") return;

        try {
            if (opcion == "1" || opcion == "2" || opcion == "3") {
                mostrarPendiente("inserción, actualización y eliminación de canciones.");
            } else if (opcion == "4") {
                importarBase();
            } else if (opcion == "5") {
                if (reabrirBase()) {
                    cout << "\n  Binario reabierto. Índice Hash reconstruido y buffer frío.\n";
                } else {
                    cout << "\n  Importa el CSV primero para crear el binario.\n";
                }
            } else {
                mostrarOpcionInvalida();
                continue;
            }
        } catch (const exception& error) {
            cout << "\n  Error: " << error.what() << '\n';
        }
        pausar();
    }
}

bool InterfazConsola::reabrirBase() {
    if (gestor_) {
        if (!gestor_->abrirAlmacenamiento()) return false;
    } else {
        gestor_.emplace("", RUTA_BINARIO, CANTIDAD_FRAMES);
        try {
            if (!gestor_->abrirAlmacenamiento()) {
                gestor_.reset();
                return false;
            }
            leerResumenBase();
        } catch (...) {
            gestor_.reset();
            throw;
        }
    }
    consultas_ = 0;
    idsAusentes_ = 0;
    return true;
}

void InterfazConsola::importarBase() {
    if (gestor_) {
        cout << "\n  El binario ya está abierto. Usa [5] para reabrirlo con el buffer frío.\n";
        return;
    }

    cout << "\n  CSV predeterminado: " << RUTA_CSV << '\n'
         << "  Destino binario:    " << RUTA_BINARIO << '\n';
    string ruta = recortar(leerLinea("  Ruta CSV (Enter para usar la predeterminada): "));
    if (!cin) return;
    if (ruta.size() >= 2 && ruta.front() == '"' && ruta.back() == '"') {
        ruta = ruta.substr(1, ruta.size() - 2);
    }
    if (ruta.empty()) ruta = RUTA_CSV;

    gestor_.emplace(ruta, RUTA_BINARIO, CANTIDAD_FRAMES);
    try {
        gestor_->importarCSV();
        leerResumenBase();
    } catch (...) {
        gestor_.reset();
        throw;
    }
    cout << "\n  Importación lista. Canciones: " << canciones_ << " · Páginas: " << paginas_ << '\n';
}

void InterfazConsola::leerResumenBase() {
    // Este recorrido cuenta registros y obtiene un ID de prueba sin tocar el Buffer Pool.
    Almacenamiento disco(RUTA_BINARIO);
    if (!disco.abrir()) {
        throw runtime_error("No se pudo leer el resumen del binario.");
    }
    paginas_ = disco.cantidadPaginas();
    canciones_ = 0;
    primerId_.reset();
    for (IdPagina id = 0; id < paginas_; ++id) {
        const auto canciones = disco.leerPagina(id).leerCanciones();
        canciones_ += canciones.size();
        if (!primerId_ && !canciones.empty()) primerId_ = canciones.front().track_id;
    }
}

// ---------------------------------------------------------------------------
// [5] Monitor del Buffer Pool
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaBufferPool() const {
    mostrarEncabezado("Monitor del Buffer Pool");
    cout << "  Capacidad: " << CANTIDAD_FRAMES << " frames · Tamaño de página: "
         << TAMANO_PAGINA_PREDETERMINADO << " bytes\n";
    if (gestor_) {
        const auto stats = gestor_->estadisticasBuffer();
        const size_t accesos = stats.hits + stats.misses;
        cout << "  HIT: " << stats.hits << " · MISS: " << stats.misses
             << " · Lecturas de página: " << stats.lecturasDisco << '\n';
        if (accesos > 0) {
            const double tasa = 100.0 * stats.hits / accesos;
            char tasaTexto[16];
            snprintf(tasaTexto, sizeof(tasaTexto), "%.1f", tasa);
            cout << "  Tasa de aciertos: " << barra(tasa, 20) << " " << tasaTexto << " %\n";
        }
    } else {
        cout << "  Importa el CSV desde Operaciones [4].\n";
    }
    pausar();
}

// ---------------------------------------------------------------------------
// [6] Estadísticas de rendimiento
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaEstadisticas() const {
    mostrarEncabezado("Estadísticas de rendimiento");
    if (gestor_) {
        imprimirTabla({"Consultas por ID", "IDs ausentes", "Lecturas de página"},
                      {18, 14, 20},
                      {{to_string(consultas_), to_string(idsAusentes_),
                        to_string(gestor_->estadisticasBuffer().lecturasDisco)}});
        cout << "\n  Lecturas = páginas cargadas por el Buffer Pool desde el archivo.\n"
             << "  El índice, el resumen y las búsquedas por texto usan lecturas directas.\n";
    } else {
        cout << "  Importa el CSV desde Operaciones [4].\n";
    }
    pausar();
}
