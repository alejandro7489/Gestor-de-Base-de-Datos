#include "GestorMusical.h"

#include <exception>
#include <iostream>

using namespace std;

int main() {
    try {
        // Construir los componentes no abre archivos ni importa el dataset.
        GestorMusical gestor;
        cout << "========================================\n"
                     "       SPOTIFY DATABASE MANAGER\n"
                     "========================================\n"
                     "Proyecto Final - Estructura de Datos\n\n"
                     "Version: 0.1\n"
                     "Integrantes:\n"
                     "- Esteban Villagarcia\n"
                     "- Fernando Lizarraga\n"
                     "- Leonardo Cuadros\n\n"
                     "Componentes preparados correctamente.\n"
                    
                     "========================================\n";
    } catch (const exception& error) {
        cerr << "Error al inicializar: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
