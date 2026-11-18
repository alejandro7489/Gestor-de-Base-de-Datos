#include "GestorMusical.h"
#include "InterfazConsola.h"
#include <exception>
#include <iostream>

using namespace std;

int main() {
    try {
        // Construir los componentes no abre archivos ni importa el dataset.
        GestorMusical gestor;
        (void)gestor;  // Aún no se pasa a la interfaz (pantallas provisionales).

        InterfazConsola interfaz;
        interfaz.ejecutar();
    } catch (const exception& error) {
        cerr << "Error al inicializar: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
