#ifndef INTERFAZ_CONSOLA_H
#define INTERFAZ_CONSOLA_H

#include "GestorMusical.h"

#include <optional>
#include <string>

class InterfazConsola {
public:
    // Muestra el menú principal y atiende al usuario hasta que elija salir.
    void ejecutar();

private:
    // --- Utilidades de presentación ---
    void prepararConsola() const;
    void limpiarPantalla() const;
    void mostrarBanner() const;
    void mostrarEstado() const;
    void mostrarMenuPrincipal() const;
    void mostrarEncabezado(const std::string& titulo) const;
    void mostrarPendiente(const std::string& detalle) const;
    void mostrarOpcionInvalida() const;
    void pausar() const;
    std::string leerLinea(const std::string& indicador) const;
    std::string leerOpcion(const std::string& contexto) const;

    // --- Pantallas (una por opción del menú) ---
    void pantallaBuscarMusica();
    void buscarIdEnConsola();
    void buscarTextoEnConsola(const std::string& opcion);
    void pantallaTop() const;
    void pantallaExplorar() const;
    void pantallaOperaciones();
    void pantallaBufferPool() const;
    void pantallaEstadisticas() const;

    bool reabrirBase();
    void importarBase();
    void leerResumenBase();

    std::optional<GestorMusical> gestor_;
    std::size_t canciones_ = 0;
    std::size_t paginas_ = 0;
    std::optional<std::string> primerId_;
    std::size_t consultas_ = 0;
    std::size_t idsAusentes_ = 0;
};

#endif // INTERFAZ_CONSOLA_H
