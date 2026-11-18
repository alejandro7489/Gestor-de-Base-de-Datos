#ifndef SPOTIFY_DBMS_LISTA_LRU_H
#define SPOTIFY_DBMS_LISTA_LRU_H

#include <cstddef>
#include <optional>

using namespace std;

class ListaLRU {
public:
    ListaLRU();

    // Pendientes: nodos propios y enlaces anterior/siguiente.
    void registrarFrame(size_t indiceFrame);
    void actualizarUso(size_t indiceFrame);
    optional<size_t> menosReciente() const;
};

#endif
