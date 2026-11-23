#include "InterfazConsola.h"
#include <exception>
#include <iostream>

using namespace std;

int main() {
    try {
        InterfazConsola interfaz;
        interfaz.ejecutar();
    } catch (const exception& error) {
        cerr << "Error al inicializar: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
