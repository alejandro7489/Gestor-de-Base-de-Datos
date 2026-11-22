#include "Almacenamiento.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace std;
namespace {
constexpr uint64_t TAMANO_CABECERA = 8;
void escribirU32(ostream& salida, uint32_t valor) {
    for (unsigned i=0;i<4;++i) salida.put(static_cast<char>((valor>>(i*8))&0xFF));
}
uint32_t leerU32(istream& entrada) {
    uint32_t valor=0;
    for(unsigned i=0;i<4;++i){ int c=entrada.get(); if(c==char_traits<char>::eof()) throw runtime_error("Cabecera binaria incompleta."); valor|=uint32_t(static_cast<unsigned char>(c))<<(i*8); }
    return valor;
}
}

Almacenamiento::Almacenamiento(const string& ruta, size_t tamanoPagina)
    : ruta_(ruta), tamanoPagina_(tamanoPagina) {
    if (tamanoPagina_ == 0 || tamanoPagina_ > numeric_limits<uint32_t>::max())
        throw invalid_argument("El tamano de pagina debe caber en uint32 y ser mayor que cero.");
}

bool Almacenamiento::abrir() {
    if (estaAbierto()) return true;
    if (!filesystem::exists(ruta_)) return false;
    archivo_.clear();
    archivo_.open(ruta_, ios::in|ios::out|ios::binary);
    if (!archivo_.is_open()) return false;
    try {
        archivo_.seekg(0,ios::beg);
        char magic[4]{};
        if(!archivo_.read(magic,4) || string(magic,4)!="SDB1") throw runtime_error("Firma binaria inválida; se esperaba SDB1.");
        uint32_t pagina=leerU32(archivo_);
        if(pagina!=tamanoPagina_) throw runtime_error("El tamaño de página del archivo no coincide con la configuración.");
        // Tras la cabecera, el tamaño debe ser múltiplo del tamaño fijo de página.
        const auto tamano=filesystem::file_size(ruta_);
        if(tamano<TAMANO_CABECERA || (tamano-TAMANO_CABECERA)%pagina!=0) throw runtime_error("El archivo binario está truncado o desalineado.");
        archivo_.clear();
        return true;
    } catch (...) {
        archivo_.close();
        archivo_.clear();
        throw;
    }
}

bool Almacenamiento::crear() {
    if (estaAbierto()) throw logic_error("Cierre el almacenamiento antes de crear un archivo.");
    filesystem::path ruta(ruta_);
    if (ruta.has_parent_path()) filesystem::create_directories(ruta.parent_path());
    // El modo app crea el archivo sin truncar uno existente.
    if (filesystem::exists(ruta_)) return false;
    ofstream salida(ruta_, ios::out | ios::binary | ios::app);
    if (!salida) return false;
    if (filesystem::file_size(ruta_) != 0) return false;
    salida.write("SDB1", 4);
    escribirU32(salida, static_cast<uint32_t>(tamanoPagina_));
    salida.flush();
    if (!salida) throw runtime_error("No se pudo escribir la cabecera del almacenamiento.");
    salida.close();
    return abrir();
}

void Almacenamiento::cerrar() { if(estaAbierto()) archivo_.close(); }
bool Almacenamiento::estaAbierto() const { return archivo_.is_open(); }
const string& Almacenamiento::ruta() const { return ruta_; }
size_t Almacenamiento::tamanoPagina() const { return tamanoPagina_; }
uintmax_t Almacenamiento::tamanoArchivo() const {
    if(!estaAbierto()) throw runtime_error("Debe abrir el almacenamiento antes de consultar su tamano.");
    return filesystem::file_size(ruta_);
}
size_t Almacenamiento::cantidadPaginas() const {
    const uintmax_t bytes=tamanoArchivo();
    if(bytes<TAMANO_CABECERA || (bytes-TAMANO_CABECERA)%tamanoPagina_!=0) throw runtime_error("El archivo binario está truncado o desalineado.");
    const uintmax_t cantidad=(bytes-TAMANO_CABECERA)/tamanoPagina_;
    if(cantidad>numeric_limits<size_t>::max()) throw overflow_error("Hay demasiadas páginas para esta plataforma.");
    return static_cast<size_t>(cantidad);
}

Pagina Almacenamiento::leerPagina(IdPagina id) {
    if(!estaAbierto()) throw logic_error("Debe abrir el almacenamiento antes de leer páginas.");
    if(id>=cantidadPaginas()) throw out_of_range("El ID de página no existe en el archivo.");
    if(id>(numeric_limits<uint64_t>::max()-TAMANO_CABECERA)/tamanoPagina_) throw overflow_error("Offset de página fuera de rango.");
    // Cada página sigue a la anterior: cabecera + ID * tamaño de página.
    const uint64_t offset=TAMANO_CABECERA+static_cast<uint64_t>(id)*tamanoPagina_;
    Pagina pagina(id,tamanoPagina_);
    archivo_.clear(); archivo_.seekg(static_cast<streamoff>(offset),ios::beg);
    if(!archivo_.read(reinterpret_cast<char*>(pagina.datos.data()),static_cast<streamsize>(pagina.datos.size()))) throw runtime_error("No se pudo leer la página completa del archivo.");
    return pagina;
}

void Almacenamiento::escribirPagina(const Pagina& pagina) {
    if(!estaAbierto()) throw logic_error("Debe abrir el almacenamiento antes de escribir páginas.");
    if(pagina.datos.size()!=tamanoPagina_) throw invalid_argument("El tamaño de la página no coincide con el archivo.");
    const size_t cantidad=cantidadPaginas();
    if(pagina.id>cantidad) throw out_of_range("No se permite dejar huecos entre páginas.");
    if(pagina.id>(numeric_limits<uint64_t>::max()-TAMANO_CABECERA)/tamanoPagina_) throw overflow_error("Offset de página fuera de rango.");
    const uint64_t offset=TAMANO_CABECERA+static_cast<uint64_t>(pagina.id)*tamanoPagina_;
    archivo_.clear(); archivo_.seekp(static_cast<streamoff>(offset),ios::beg);
    if(!archivo_.write(reinterpret_cast<const char*>(pagina.datos.data()),static_cast<streamsize>(pagina.datos.size()))) throw runtime_error("No se pudo escribir la página completa.");
    archivo_.flush();
    if(!archivo_) throw runtime_error("Error al finalizar la escritura de la página.");
}
