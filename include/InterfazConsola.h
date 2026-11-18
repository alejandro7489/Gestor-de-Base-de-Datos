#ifndef INTERFAZ_CONSOLA_H
#define INTERFAZ_CONSOLA_H

#include <string>

// Interfaz de consola PROVISIONAL del Spotify Database Manager.
//
// Todas las pantallas muestran datos de ejemplo: todavía no se conectan con
// GestorMusical, BufferPool ni LectorCSV. Cuando cada componente esté listo,
// basta con reemplazar el contenido de la pantalla correspondiente.
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
    void pantallaBuscarMusica() const;
    void pantallaTop() const;
    void pantallaExplorar() const;
    void pantallaOperaciones() const;
    void pantallaBufferPool() const;
    void pantallaEstadisticas() const;
};

#endif // INTERFAZ_CONSOLA_H