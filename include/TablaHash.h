#ifndef SPOTIFY_DBMS_TABLA_HASH_H
#define SPOTIFY_DBMS_TABLA_HASH_H

#include <cstddef>
#include <optional>
#include <string>

using namespace std;

// Contrato futuro: clave -> posicion. Cada instancia tendra un unico uso:
// track_id -> ID de pagina, o ID de pagina como texto -> indice del frame.
class TablaHash {
public:
    explicit TablaHash(size_t capacidad = 101);
    size_t capacidad() const;

    // Pendientes: funcion hash, almacenamiento de entradas y manejo de colisiones.
    void insertar(const string& clave, size_t posicion);
    optional<size_t> buscar(const string& clave) const;
    bool eliminar(const string& clave);

private:
    size_t capacidad_;
};

#endif
