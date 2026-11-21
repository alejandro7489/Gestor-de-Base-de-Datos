#include "TablaHash.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {
constexpr size_t CANTIDAD_ESPERADA = 1000;

// Lector de registros para esta prueba. Una coma o un salto de línea
// entre comillas pertenece al campo. Dos comillas seguidas representan una.
bool leerRegistro(istream& csv, vector<string>& campos) {
    campos.clear();
    string campo;
    bool entreComillas = false;
    char caracter;

    while (csv.get(caracter)) {
        if (entreComillas) {
            if (caracter == '"' && csv.peek() == '"') {
                csv.get();
                campo += '"';
            } else if (caracter == '"') {
                entreComillas = false;
            } else {
                campo += caracter;
            }
        } else if (caracter == '"' && campo.empty()) {
            entreComillas = true;
        } else if (caracter == ',') {
            campos.push_back(campo);
            campo.clear();
        } else if (caracter == '\n' || caracter == '\r') {
            if (caracter == '\r' && csv.peek() == '\n') {
                csv.get();
            }
            campos.push_back(campo);
            return true;
        } else {
            campo += caracter;
        }
    }

    if (csv.bad() || entreComillas) {
        throw runtime_error("Error de lectura o campo sin comillas de cierre.");
    }
    if (campos.empty() && campo.empty()) {
        return false;
    }
    campos.push_back(campo);
    return true;
}

vector<string> leerIds(const string& ruta) {
    ifstream csv(ruta, ios::binary);
    if (!csv) {
        throw runtime_error("No se pudo abrir " + ruta);
    }
    if (csv.peek() == 0xEF) {
        // Algunos CSV en UTF-8 empiezan con una marca de tres bytes.
        char marca[3]{};
        if (!csv.read(marca, 3) || string(marca, 3) != "\xEF\xBB\xBF") {
            throw runtime_error("Marca UTF-8 inválida al inicio del CSV.");
        }
    }

    vector<string> campos;
    if (!leerRegistro(csv, campos)) {
        throw runtime_error("El CSV no tiene encabezado.");
    }
    const auto columna = find(campos.begin(), campos.end(), "track_id");
    if (columna == campos.end()) {
        throw runtime_error("El encabezado no contiene la columna track_id.");
    }
    const size_t indiceId = static_cast<size_t>(columna - campos.begin());
    const size_t cantidadColumnas = campos.size();

    vector<string> ids;
    while (leerRegistro(csv, campos)) {
        if (campos.size() != cantidadColumnas || campos[indiceId].empty()) {
            throw runtime_error("Columnas incorrectas o track_id vacío en el registro "
                                + to_string(ids.size() + 1));
        }
        ids.push_back(campos[indiceId]);
    }
    return ids;
}

void comprobar(bool condicion, const string& mensaje) {
    if (!condicion) {
        throw runtime_error(mensaje);
    }
}

void probarHash(const vector<string>& ids) {
    comprobar(ids.size() == CANTIDAD_ESPERADA, "Se esperaban 1000 registros.");
    TablaHash tabla;

    // La posición temporal es el índice del registro en el CSV, desde cero.
    const auto inicioInsercion = chrono::steady_clock::now();
    for (size_t indice = 0; indice < ids.size(); ++indice) {
        tabla.insertar(ids[indice], indice);
    }
    const auto finInsercion = chrono::steady_clock::now();

    size_t aciertos = 0;
    const auto inicioBusqueda = chrono::steady_clock::now();
    for (size_t indice = 0; indice < ids.size(); ++indice) {
        if (tabla.buscar(ids[indice]) != indice) {
            throw runtime_error("Índice incorrecto para el registro " + to_string(indice)
                                + " (" + ids[indice] + ").");
        }
        ++aciertos;
    }
    const auto finBusqueda = chrono::steady_clock::now();

    for (const string idAusente : {"#id-ausente-1#", "#id-ausente-2#"}) {
        comprobar(find(ids.begin(), ids.end(), idAusente) == ids.end(),
                  "La clave elegida para probar ausencia está en el CSV.");
        comprobar(!tabla.buscar(idAusente), "Se encontró un ID inexistente.");
    }

    const size_t indiceActualizado = ids.size() / 2;
    const size_t nuevoValor = ids.size();
    tabla.insertar(ids[indiceActualizado], nuevoValor);
    comprobar(tabla.buscar(ids[indiceActualizado]) == nuevoValor, "Falló la actualización.");

    const size_t indiceEliminado = ids.size() - 1;
    comprobar(tabla.eliminar(ids[indiceEliminado]), "Falló la eliminación.");
    comprobar(!tabla.buscar(ids[indiceEliminado]), "El ID eliminado sigue en la tabla.");
    comprobar(!tabla.eliminar(ids[indiceEliminado]), "El ID eliminado solo debe existir una vez.");

    for (size_t indice = 0; indice < ids.size(); ++indice) {
        optional<size_t> esperado;
        if (indice != indiceEliminado) {
            esperado = indice == indiceActualizado ? nuevoValor : indice;
        }
        if (tabla.buscar(ids[indice]) != esperado) {
            throw runtime_error("Valor incorrecto después de los cambios para " + ids[indice]);
        }
    }

    using Microsegundos = chrono::duration<double, micro>;
    cout << "Registros leídos: " << ids.size() << '\n'
         << "IDs insertados: " << ids.size() << '\n'
         << "Búsquedas con el índice correcto antes de los cambios: " << aciertos << '\n'
         << "IDs inexistentes no encontrados: 2 de 2\n"
         << "Actualización: correcta (índice " << indiceActualizado << " -> " << nuevoValor << ")\n"
         << "Eliminación: correcta (índice " << indiceEliminado << ")\n"
         << "Comprobaciones después de los cambios: " << ids.size() << " de " << ids.size() << '\n'
         << fixed << setprecision(1)
         << "Tiempo de inserción: " << Microsegundos(finInsercion - inicioInsercion).count() << " us\n"
         << "Tiempo de búsqueda: " << Microsegundos(finBusqueda - inicioBusqueda).count() << " us\n";
}
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: hash_dataset <ruta-del-CSV>\n";
        return 2;
    }

    try {
        probarHash(leerIds(argv[1]));
        return 0;
    } catch (const exception& error) {
        cerr << "Falló la prueba con el dataset: " << error.what() << '\n';
        return 1;
    }
}
