#include "Almacenamiento.h"
#include "LectorCSV.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace std;
namespace {
void comprobar(bool ok,const string& mensaje){ if(!ok) throw runtime_error(mensaje); }
struct Temporales {
    filesystem::path dir;
    Temporales(){ dir=filesystem::temp_directory_path()/("spotify-storage-"+to_string(chrono::steady_clock::now().time_since_epoch().count())); comprobar(filesystem::create_directory(dir),"No se creó directorio temporal."); }
    ~Temporales(){ error_code e; filesystem::remove_all(dir,e); }
};
string encabezado(){ return "name,track_artists,album_name,album_release_date,duration_ms,genres,explicit,track_id,track_track_number,album_total_tracks,streams,artist_followers,artist_popularity,track_album_album,popularity,tempo\n"; }
void probarCSV(const filesystem::path& dir){
    const auto ruta=dir/"entrada.csv";
    { ofstream f(ruta,ios::binary); f<<"\xEF\xBB\xBF"<<encabezado()<<"\"Tema, uno\",Artista,Álbum,2024,123.0,,true,id-1,1.0,12,999,45,80,Álbum,90,123.5\n"<<"\"Nombre\ncon línea\",A,B,2020,,,,id-2,,,,,,, ,\n"; }
    LectorCSV lector(ruta.string()); comprobar(lector.abrir(),"No abrió CSV.");
    const auto a=lector.leerSiguiente(); comprobar(a && a->name=="Tema, uno" && a->duration_ms==123 && a->es_explicita==true && a->tempo==123.5,"No parseó comillas/números.");
    const auto b=lector.leerSiguiente(); comprobar(b && b->name=="Nombre\ncon línea" && !b->streams,"No conservó salto o campo vacío.");
    comprobar(!lector.leerSiguiente(),"EOF debe devolver nullopt."); lector.cerrar();
    const auto malo=dir/"malo.csv"; {ofstream f(malo); f<<encabezado()<<"a,b,c,d,1.2,,,,,,,,,,,\n";}
    LectorCSV lectorMalo(malo.string()); comprobar(lectorMalo.abrir(),"No abrió CSV inválido."); bool error=false; try{(void)lectorMalo.leerSiguiente();}catch(const runtime_error& e){error=string(e.what()).find("registro CSV")!=string::npos;} comprobar(error,"Debe señalar registro inválido.");
}
void probarPaginaYArchivo(const filesystem::path& dir){
    Cancion c; c.name="Tema"; c.track_artists="Artista"; c.album_name="Álbum"; c.album_release_date="2024"; c.duration_ms=123; c.genres="pop"; c.es_explicita=false; c.track_id="id-1"; c.track_track_number=1; c.album_total_tracks=10; c.streams=999; c.artist_followers=45; c.artist_popularity=80; c.track_album_album="Álbum"; c.popularity=90; c.tempo=123.5;
    Pagina pagina(0,512); comprobar(pagina.agregarCancion(c),"No agregó canción.");
    const auto canciones=pagina.leerCanciones(); comprobar(canciones.size()==1 && canciones[0].track_id==c.track_id && canciones[0].tempo==c.tempo && canciones[0].es_explicita==c.es_explicita,"Falló round-trip de página.");
    Cancion grande; grande.track_id="no-cabe"; grande.name=string(1000,'x'); comprobar(!pagina.agregarCancion(grande) && pagina.leerCanciones().size()==1,"Página debe rechazar sin modificar cuando no cabe.");
    const auto ruta=dir/"datos.bin"; Almacenamiento db(ruta.string(),512); comprobar(db.crear(),"No creó binario."); comprobar(!db.crear(),"crear no debe sobrescribir existente."); db.escribirPagina(pagina); comprobar(db.cantidadPaginas()==1,"Cantidad de páginas incorrecta."); db.cerrar();
    Almacenamiento reabierto(ruta.string(),512); comprobar(reabierto.abrir(),"No reabrió binario."); const auto recuperada=reabierto.leerPagina(0).leerCanciones(); comprobar(recuperada.size()==1 && recuperada[0].track_id=="id-1","Falló lectura del binario."); reabierto.cerrar();
    Almacenamiento incompatible(ruta.string(),256); bool error=false; try{(void)incompatible.abrir();}catch(const runtime_error&){error=true;} comprobar(error,"Debe rechazar tamaño incompatible.");
}
}
int main(){try{Temporales t; probarCSV(t.dir); probarPaginaYArchivo(t.dir); cout<<"Pasaron las pruebas de Storage.\n"; return 0;}catch(const exception& e){cerr<<"Falló Storage: "<<e.what()<<'\n'; return 1;}}
