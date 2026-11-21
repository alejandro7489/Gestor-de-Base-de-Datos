#include "TablaHash.h"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace std;

namespace {
void comprobar(bool condicion, const string& mensaje) {
    if (!condicion) {
        throw runtime_error(mensaje);
    }
}

void probarInsercionYBusqueda() {
    TablaHash tabla;
    comprobar(tabla.capacidad() == 101, "La capacidad por defecto debe ser 101.");
    tabla.insertar("cancion", 0);
    tabla.insertar("cancion-extra", 15);

    comprobar(tabla.buscar("cancion") == 0, "Debe recuperar la posición cero.");
    comprobar(tabla.buscar("cancion-extra") == 15, "Debe recuperar la segunda clave.");
    comprobar(!tabla.buscar("canci"), "Una coincidencia parcial no es suficiente.");
    comprobar(!tabla.buscar("CANCION"), "La búsqueda debe respetar mayúsculas.");
    comprobar(!tabla.buscar("inexistente"), "Una clave ausente debe devolver nullopt.");
}

void probarColisionesYEliminacion() {
    TablaHash tabla(1);
    tabla.insertar("a", 1);
    tabla.insertar("b", 2);
    tabla.insertar("c", 3);
    tabla.insertar("d", 4);
    comprobar(tabla.capacidad() == 1, "Debe aceptar cuatro claves en un bucket.");
    comprobar(tabla.buscar("a") == 1 && tabla.buscar("b") == 2
              && tabla.buscar("c") == 3 && tabla.buscar("d") == 4,
              "Una colisión no debe perder ninguna clave.");

    tabla.insertar("b", 20);
    comprobar(tabla.buscar("b") == 20, "Reinsertar debe actualizar la posición.");

    // El orden de esta cadena es d -> c -> b -> a.
    comprobar(tabla.eliminar("d") && !tabla.buscar("d"), "Falló eliminar la cabeza.");
    comprobar(tabla.eliminar("b") && !tabla.buscar("b"), "Falló eliminar el nodo intermedio.");
    comprobar(tabla.eliminar("a") && !tabla.buscar("a"), "Falló eliminar el último nodo.");
    comprobar(!tabla.eliminar("a"), "Eliminar una clave ausente debe devolver false.");
    comprobar(tabla.buscar("c") == 3, "Las eliminaciones deben conservar los otros nodos.");
    comprobar(tabla.eliminar("c") && !tabla.buscar("c"), "Debe poder vaciar el bucket.");
    comprobar(!tabla.eliminar("c"), "Un bucket vacío debe devolver false al eliminar.");
    tabla.insertar("c", 30);
    comprobar(tabla.buscar("c") == 30, "Debe poder reutilizar un bucket vacío.");
}

void probarCasosLimite() {
    bool rechazoCapacidadCero = false;
    try {
        TablaHash tabla(0);
    } catch (const invalid_argument&) {
        rechazoCapacidadCero = true;
    }
    comprobar(rechazoCapacidadCero, "La capacidad cero debe producir invalid_argument.");

    TablaHash tabla(7);
    const string claveConNulo("a\0b", 3);
    const size_t posicionMaxima = numeric_limits<size_t>::max();
    comprobar(!tabla.buscar(""), "Una tabla vacía no debe encontrar la clave vacía.");
    tabla.insertar("", posicionMaxima);
    tabla.insertar(claveConNulo, 5);
    tabla.insertar("a", 6);
    tabla.insertar(u8"canción", 7);
    comprobar(tabla.buscar("") == posicionMaxima, "Debe admitir clave vacía y posición máxima.");
    comprobar(tabla.buscar(claveConNulo) == 5 && tabla.buscar("a") == 6,
              "La comparación debe incluir todos los bytes de la clave.");
    comprobar(tabla.buscar(u8"canción") == 7, "Debe admitir claves UTF-8.");
}

void probarCopias() {
    TablaHash original(1);
    original.insertar("a", 1);
    original.insertar("b", 2);

    TablaHash copia(original);
    comprobar(copia.buscar("a") == 1 && copia.buscar("b") == 2,
              "La copia debe conservar ambas claves.");
    copia.insertar("a", 3);
    copia.eliminar("b");
    comprobar(original.buscar("a") == 1 && original.buscar("b") == 2,
              "Las modificaciones en la copia deben ser independientes.");

    TablaHash asignada(5);
    asignada.insertar("anterior", 9);
    asignada = original;
    comprobar(asignada.capacidad() == 1 && !asignada.buscar("anterior"),
              "La asignación debe sustituir los buckets anteriores.");
    original.eliminar("a");
    comprobar(asignada.buscar("a") == 1 && asignada.buscar("b") == 2,
              "La asignación debe crear sus propios nodos.");
}
}

int main() {
    try {
        probarInsercionYBusqueda();
        probarColisionesYEliminacion();
        probarCasosLimite();
        probarCopias();
        cout << "Pasaron las 4 pruebas de TablaHash.\n";
        return 0;
    } catch (const exception& error) {
        cerr << "Falló una prueba de TablaHash: " << error.what() << '\n';
        return 1;
    }
}
