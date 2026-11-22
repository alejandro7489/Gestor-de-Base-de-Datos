#ifndef SPOTIFY_DBMS_ALMACENAMIENTO_H
#define SPOTIFY_DBMS_ALMACENAMIENTO_H

#include "Pagina.h"

#include <cstdint>
#include <fstream>
#include <string>

using namespace std;

class Almacenamiento {
public:
    explicit Almacenamiento(const string& ruta,
                           size_t tamanoPagina = TAMANO_PAGINA_PREDETERMINADO);

    // Solo abre archivos existentes. No crea el archivo binario.
    bool abrir();
    bool crear();
    void cerrar();
    bool estaAbierto() const;
    const string& ruta() const;
    size_t tamanoPagina() const;
    uintmax_t tamanoArchivo() const;
    size_t cantidadPaginas() const;

    Pagina leerPagina(IdPagina id);
    void escribirPagina(const Pagina& pagina);

private:
    string ruta_;
    size_t tamanoPagina_;
    fstream archivo_;
};

#endif
