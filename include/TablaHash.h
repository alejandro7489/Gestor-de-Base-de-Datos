#ifndef SPOTIFY_DBMS_TABLA_HASH_H
#define SPOTIFY_DBMS_TABLA_HASH_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using namespace std;

// Cada clave completa se asocia con una posición, por ejemplo, el ID de una página.
class TablaHash {
public:
    explicit TablaHash(size_t capacidad = 101);
    TablaHash(const TablaHash& otra);
    TablaHash& operator=(const TablaHash& otra);
    ~TablaHash();

    size_t capacidad() const;

    void insertar(const string& clave, size_t posicion);
    optional<size_t> buscar(const string& clave) const;
    bool eliminar(const string& clave);

private:
    struct Nodo {
        string clave;
        size_t posicion;
        Nodo* siguiente;
    };

    size_t indiceBucket(const string& clave) const;
    vector<Nodo*> buckets_;
};

#endif
