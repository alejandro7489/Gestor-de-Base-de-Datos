#include "InterfazConsola.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

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

// Cifras ILUSTRATIVAS de la pantalla principal (no son resultados reales).
const string DEMO_TRACKS  = "897,432";
const string DEMO_PAGINAS = "31,204";
const string DEMO_BUFFER  = "4 MiB";
const string DEMO_PAGINA  = "4 KiB";

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

string dosDigitos(int n) {
    return (n < 10 ? "0" : "") + to_string(n);
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
    lineas.push_back(VERDE + NEGRITA + "● BASE DE DATOS EN LÍNEA" + RESET);
    lineas.push_back("Canciones: " + DEMO_TRACKS + " · Páginas: " + DEMO_PAGINAS);
    lineas.push_back("Buffer: " + DEMO_BUFFER + " · Tamaño de página: " + DEMO_PAGINA);
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
    cout << GRIS << repetir("─", ANCHO_PANTALLA) << RESET << "\n";
    cout << "  " << AMARILLO << "Pantalla provisional · los datos mostrados son de ejemplo" << RESET << "\n\n";
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
void InterfazConsola::pantallaBuscarMusica() const {
    while (true) {
        mostrarEncabezado("Buscar música");
        cout << "  " << VERDE << "[1]" << RESET << "  Por ID de canción (track_id)\n"
             << "  " << VERDE << "[2]" << RESET << "  Por artista\n"
             << "  " << VERDE << "[3]" << RESET << "  Por álbum\n"
             << "  " << VERDE << "[4]" << RESET << "  Filtrar por año o género\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        string opcion = leerOpcion("buscar");
        if (opcion == "0") return;
        if (opcion.size() != 1 || opcion[0] < '1' || opcion[0] > '4') {
            mostrarOpcionInvalida();
            continue;
        }

        string criterio = recortar(leerLinea("\n  Texto a buscar: "));
        cout << "\n  Resultados de ejemplo para \"" << criterio << "\":\n\n";

        vector<vector<string>> filas;
        for (int i = 1; i <= 3; i++) {
            vector<string> fila;
            fila.push_back("Track de ejemplo " + dosDigitos(i));
            fila.push_back("Artista de ejemplo " + string(1, static_cast<char>('A' + i - 1)));
            fila.push_back(to_string(90 - i * 3));
            filas.push_back(fila);
        }
        imprimirTabla({"Nombre", "Artista", "Popularidad"}, {24, 20, 12}, filas);

        mostrarPendiente("la búsqueda real se conectará con GestorMusical / LectorCSV.");
        pausar();
    }
}

// ---------------------------------------------------------------------------
// [2] Top de popularidad
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaTop() const {
    while (true) {
        mostrarEncabezado("Top de popularidad");
        cout << "  " << VERDE << "[1]" << RESET << "  Top global\n"
             << "  " << VERDE << "[2]" << RESET << "  Top por género\n"
             << "  " << VERDE << "[3]" << RESET << "  Top por año\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        string opcion = leerOpcion("top");
        if (opcion == "0") return;
        if (opcion != "1" && opcion != "2" && opcion != "3") {
            mostrarOpcionInvalida();
            continue;
        }

        string titulo = opcion == "1" ? "Top global"
                      : opcion == "2" ? "Top por género"
                                      : "Top por año";
        mostrarEncabezado(titulo);

        vector<vector<string>> filas;
        for (int i = 0; i < 10; i++) {
            vector<string> fila;
            fila.push_back("#" + to_string(i + 1));
            fila.push_back("Track de ejemplo " + dosDigitos(i + 1));
            fila.push_back("Artista de ejemplo " + string(1, static_cast<char>('A' + i)));
            fila.push_back(to_string(98 - i));
            filas.push_back(fila);
        }
        imprimirTabla({"Pos.", "Nombre", "Artista", "Popularidad"}, {4, 20, 18, 12}, filas);

        mostrarPendiente("el ranking real se calculará con un Heap (fase posterior).");
        pausar();
    }
}

// ---------------------------------------------------------------------------
// [3] Explorar biblioteca
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaExplorar() const {
    while (true) {
        mostrarEncabezado("Explorar biblioteca");
        cout << "  " << VERDE << "[1]" << RESET << "  Artistas\n"
             << "  " << VERDE << "[2]" << RESET << "  Álbumes\n"
             << "  " << VERDE << "[3]" << RESET << "  Géneros\n"
             << "  " << VERDE << "[4]" << RESET << "  Años de lanzamiento\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        string opcion = leerOpcion("explorar");
        if (opcion == "0") return;
        if (opcion.size() != 1 || opcion[0] < '1' || opcion[0] > '4') {
            mostrarOpcionInvalida();
            continue;
        }

        string titulo, prefijo;
        switch (opcion[0]) {
            case '1': titulo = "Artistas";            prefijo = "Artista de ejemplo "; break;
            case '2': titulo = "Álbumes";             prefijo = "Álbum de ejemplo ";   break;
            case '3': titulo = "Géneros";             prefijo = "Género de ejemplo ";  break;
            default:  titulo = "Años de lanzamiento"; prefijo = "";                    break;
        }
        mostrarEncabezado("Explorar · " + titulo);

        vector<vector<string>> filas;
        for (int i = 0; i < 6; i++) {
            vector<string> fila;
            if (opcion[0] == '4') fila.push_back(to_string(2024 - i));
            else                  fila.push_back(prefijo + string(1, static_cast<char>('A' + i)));
            fila.push_back(to_string(1200 - i * 175));
            filas.push_back(fila);
        }
        imprimirTabla({titulo, "Canciones"}, {28, 12}, filas);

        mostrarPendiente("la exploración real dependerá de los índices (hash / AVL).");
        pausar();
    }
}

// ---------------------------------------------------------------------------
// [4] Operaciones de base de datos
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaOperaciones() const {
    while (true) {
        mostrarEncabezado("Operaciones de base de datos");
        cout << "  " << VERDE << "[1]" << RESET << "  Insertar canción\n"
             << "  " << VERDE << "[2]" << RESET << "  Actualizar canción\n"
             << "  " << VERDE << "[3]" << RESET << "  Eliminar canción\n"
             << "  " << VERDE << "[4]" << RESET << "  Importar CSV\n\n"
             << "  " << VERDE << "[0]" << RESET << "  Volver\n\n";

        string opcion = leerOpcion("ops");
        if (opcion == "0") return;

        if (opcion == "1") {
            leerLinea("\n  track_id : ");
            leerLinea("  name     : ");
            leerLinea("  artista  : ");
            mostrarPendiente("la inserción real requiere almacenamiento paginado e índice hash.");
            cout << "  " << GRIS << "(No se guardó ningún dato.)" << RESET << "\n";
        } else if (opcion == "2") {
            leerLinea("\n  track_id a actualizar : ");
            mostrarPendiente("la actualización real requiere almacenamiento paginado.");
            cout << "  " << GRIS << "(No se modificó ningún dato.)" << RESET << "\n";
        } else if (opcion == "3") {
            leerLinea("\n  track_id a eliminar : ");
            leerLinea("  ¿Confirmar eliminación? (s/n) : ");
            mostrarPendiente("la eliminación real requiere almacenamiento paginado.");
            cout << "  " << GRIS << "(No se eliminó ningún dato.)" << RESET << "\n";
        } else if (opcion == "4") {
            cout << "\n  Origen  : ../spotify_reduced.csv\n"
                 << "  Destino : data/canciones.bin\n";
            mostrarPendiente("la importación necesita el parser incremental y el formato binario.");
        } else {
            mostrarOpcionInvalida();
            continue;
        }
        pausar();
    }
}

// ---------------------------------------------------------------------------
// [5] Monitor del Buffer Pool
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaBufferPool() const {
    mostrarEncabezado("Monitor del Buffer Pool");

    // Estado de ejemplo: ID de página por frame (-1 = frame libre).
    const int paginas[16] = {140, 87, 2031, 15, 988, 4, 760, 312, 55, 1204,
                             -1, -1, -1, -1, -1, -1};
    int ocupados = 0;
    for (int i = 0; i < 16; i++) if (paginas[i] >= 0) ocupados++;

    cout << "  Capacidad : 16 frames · Tamaño de página: 4096 bytes\n";
    cout << "  Ocupados  : " << ocupados << " / 16\n\n";

    // Cuadrícula de 4x4 frames
    for (int i = 0; i < 16; i++) {
        bool ocupado = paginas[i] >= 0;
        string celda = "[F" + dosDigitos(i) + ":" + (ocupado ? "P" + to_string(paginas[i]) : string("libre")) + "]";
        if (i % 4 == 0) cout << "  ";
        cout << (ocupado ? VERDE : GRIS) << rellenar(celda, 15) << RESET;
        if (i % 4 == 3) cout << "\n";
    }

    // Hits y misses de ejemplo
    const int aciertos = 1284, fallos = 216;
    double tasa = 100.0 * aciertos / (aciertos + fallos);
    char tasaTexto[16];
    snprintf(tasaTexto, sizeof(tasaTexto), "%.1f", tasa);

    cout << "\n  Aciertos (hits)  : " << aciertos << "\n";
    cout << "  Fallos (misses)  : " << fallos << "\n";
    cout << "  Tasa de aciertos : " << barra(tasa, 20) << " " << tasaTexto << " %\n\n";
    cout << "  Orden LRU (más reciente → menos reciente):\n";
    cout << "  " << GRIS << "P140 → P87 → P2031 → P15 → P988 → … → P1204" << RESET << "\n";

    mostrarPendiente("estos valores se tomarán de BufferPool cuando implemente hits/misses y LRU.");
    pausar();
}

// ---------------------------------------------------------------------------
// [6] Estadísticas de rendimiento
// ---------------------------------------------------------------------------
void InterfazConsola::pantallaEstadisticas() const {
    mostrarEncabezado("Estadísticas de rendimiento");

    vector<vector<string>> filas;
    filas.push_back({"Búsqueda por ID (hash)",        "0.42 ms",   "1,000"});
    filas.push_back({"Búsqueda secuencial (CSV)",     "1,840 ms",  "1,000"});
    filas.push_back({"Lectura de página · hit",       "0.02 ms",   "1,000"});
    filas.push_back({"Lectura de página · miss",      "3.10 ms",   "1,000"});
    filas.push_back({"Inserción de registro",         "0.90 ms",   "1,000"});
    imprimirTabla({"Operación", "Tiempo prom.", "Muestras"}, {28, 12, 10}, filas);

    mostrarPendiente("las mediciones reales se registrarán cuando existan las operaciones.");
    pausar();
}