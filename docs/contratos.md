# Contratos técnicos compartidos — Storage + Buffer Pool

> **IMPORTANTE — LECTURA OBLIGATORIA PARA TODO EL EQUIPO Y SUS ASISTENTES DE IA.** Este documento contiene las decisiones de integración acordadas para la demostración del lunes **23 de noviembre de 2026**. No cambiar firmas públicas o el formato de páginas sin aprobación de los tres integrantes.

Relacionado: [Plan y responsabilidades](PLAN_DE_TRABAJO_IA.md) · [Arquitectura existente](arquitectura.md)

## 1. Reglas generales

- Lenguaje: **C++17**. Conservar el scaffold que ya existe en `main`; **no regenerarlo**.
- CSV de entrada: `spotify_reduced.csv`; para pruebas, `spotify_sample_1000.csv` si está disponible localmente. Ningún CSV ni binario pesado debe subirse a Git.
- Página de **4096 bytes por defecto**, configurable antes de importar; todas las páginas de un mismo archivo tienen igual tamaño.
- Cantidad de frames del Buffer Pool configurable e independiente del tamaño del disco.
- Para la primera entrega, consultas de **solo lectura**. **LRU, CLOCK, MRU, AVL, Heap, SQL y CRUD completo quedan fuera del alcance**.
- Los métodos existentes con `logic_error("Pendiente")` no representan funcionalidades implementadas.

## 2. Contrato del CSV

Estas son las 16 columnas, en el **orden exacto**:

```text
name,track_artists,album_name,album_release_date,duration_ms,genres,explicit,track_id,track_track_number,album_total_tracks,streams,artist_followers,artist_popularity,track_album_album,popularity,tempo
```

`LectorCSV::leerSiguiente()` devuelve una `std::optional<Cancion>`: una canción por llamada y `std::nullopt` exclusivamente al llegar a EOF. No cargar todo el CSV en memoria. Validar encabezado, campos con comas, comillas escapadas, saltos de línea entrecomillados, UTF-8, campos opcionales vacíos, rangos de enteros y valores como `123.0` solo si representan enteros exactos. Si la entrada es inválida, comunicar el número de registro, sin descartarla silenciosamente. El nombre C++ de `explicit` es `es_explicita`.

## 3. Contrato del archivo binario

**Cabecera de 8 bytes**, seguida por páginas contiguas:

| Offset | Longitud | Contenido |
|---|---:|---|
| `0` | 4 bytes | ASCII `SDB1` |
| `4` | 4 bytes | Tamaño de página `uint32_t` little-endian |
| `8` | N × tamaño de página | Páginas consecutivas, IDs desde 0 |

Offset de una página `id`: `8 + id * tamanoPagina`.

Validar tamaño configurado, páginas completas y alineación `(tamanoArchivo - 8) % tamanoPagina == 0`. `abrir()` abre un archivo existente sin crearlo ni truncarlo; el método nuevo `crear()` crea un archivo **solo si no existe**. `escribirPagina()` puede sobrescribir una página válida o agregar la siguiente página contigua; no dejar huecos.

### Estructura de una página

```text
[recordCount: uint32 LE]
[recordLength: uint32 LE][recordPayload...]
[recordLength: uint32 LE][recordPayload...]
...
[padding con ceros hasta completar tamanoPagina]
```

Una página almacena **varios registros de longitud variable**. No cortar una canción entre páginas. `Pagina::agregarCancion()` devuelve `false` sin modificar la página si el registro no cabe; el importador inicia una página nueva. Si tampoco cabe en una página vacía, devolver error claro que incluya el `track_id`. `Pagina::leerCanciones()` debe validar las longitudes antes de leer.

### Serialización de canción

Se serializan los 16 campos **en el orden del CSV**, no los bytes crudos de un objeto `Cancion`:

- `string`: `uint32 LE` de longitud seguido por esa cantidad de bytes UTF-8.
- `optional<T>`: un byte de presencia 0/1, seguido por el valor solo si está presente.
- Enteros `uint32_t`, `uint64_t`, `int32_t`: ancho fijo, little-endian, rango validado.
- `bool`: byte 0/1 (después del byte de presencia, al ser opcional).
- `tempo` (`double`): representación IEEE-754 binary64, 8 bytes little-endian; no usar `sizeof(Cancion)` como formato persistente.

Los tipos de cada campo deben seguir `include/Cancion.h`: `duration_ms`, `streams`, `artist_followers` son `optional<uint64_t>`; `track_track_number`, `album_total_tracks`, `optional<uint32_t>`; `artist_popularity`, `popularity`, `optional<int>` (codificar como int32); `es_explicita`, `optional<bool>`; `tempo`, `optional<double>`; el resto, cadenas.

**Este formato es una decisión de nuestro equipo** para simplificar la implementación; no es una especificación oficial de Spotify ni un formato proporcionado por la docente.

## 4. Interfaces compartidas

Las firmas existentes se mantienen salvo acuerdos expresos. Nuevas funciones señaladas con `// nueva`:

```cpp
// Fernando: CSV, página y almacenamiento
std::optional<Cancion> LectorCSV::leerSiguiente();
bool Pagina::agregarCancion(const Cancion& cancion);       // nueva
std::vector<Cancion> Pagina::leerCanciones() const;       // nueva
bool Almacenamiento::abrir();
bool Almacenamiento::crear();                             // nueva
std::size_t Almacenamiento::cantidadPaginas() const;      // nueva
Pagina Almacenamiento::leerPagina(IdPagina id);
void Almacenamiento::escribirPagina(const Pagina& pagina);

// Leonardo: Buffer Pool
const Pagina& BufferPool::obtenerPagina(IdPagina id, Almacenamiento& almacenamiento);
void BufferPool::liberarPagina(IdPagina id);              // nueva
EstadisticasBuffer BufferPool::estadisticas() const;

// Esteban: Hash y gestor
void TablaHash::insertar(const std::string& clave, std::size_t posicion);
std::optional<std::size_t> TablaHash::buscar(const std::string& clave) const;
bool TablaHash::eliminar(const std::string& clave);
void GestorMusical::importarCSV();
std::optional<Cancion> GestorMusical::buscarPorId(const std::string& trackId);
```

### Buffer Pool: semántica obligatoria

- `Frame` contiene su `Pagina`, el campo `ocupado`, `pinCount = 0` y `dirtyBit = false`. El `frameId` puede representarse mediante el índice en el array, sin duplicarlo.
- **Hit:** la página ya está en un frame; incrementar hit y pin; **no leer disco**.
- **Miss exitoso:** página ausente; leer exactamente esa página del binario, cargar en un frame elegible, incrementar miss y pin.
- Reemplazo temporal **NO LRU**: usar primero un frame vacío; después el **primer frame limpio y no fijado** (`pinCount == 0 && dirtyBit == false`), recorriendo por índice.
- Si todos los frames están fijados o no hay víctima elegible, fallar de manera controlada sin alterar datos; solicitudes fallidas no se cuentan como hits/misses exitosos.
- Cada solicitud exitosa debe tener **exactamente una liberación** mediante `liberarPagina()`. No permitir liberar sin pin. La referencia devuelta solo es válida mientras la página está fijada.
- En esta primera fase no se modifica información en el buffer; sus páginas cargadas comienzan limpias y permanecen así. Las escrituras/dirty flush se implementarán después.
- La búsqueda de páginas residentes puede ser **lineal temporalmente** para desacoplar el módulo de Leonardo del Hash de Esteban.

### Hash y consultas

- `TablaHash`: implementación propia con buckets y resolución de colisiones por encadenamiento; claves completas, inserciones repetidas **actualizan**. Capacidad constructor = cantidad de buckets, no número máximo de entradas.
- Índice principal `track_id → pageId`, no una copia de las canciones. Los registros dentro de una página se identifican comparando `track_id` al decodificarlos.
- IDs repetidos durante la importación: **conservar la primera aparición**.
- Al reabrir el archivo binario, reconstruir el Hash recorriendo páginas de disco directamente, sin llenar de antemano el Buffer Pool: la primera consulta debe poder producir un miss.
- Un ID inexistente devuelve `nullopt` sin solicitar páginas ni modificar estadísticas. Una consulta exitosa debe liberar su pin incluso si la decodificación falla.

## 5. Pruebas de aceptación

| Prueba | Resultado esperado |
|---|---|
| CSV entrecomillado, vacíos, `123.0`, error en registro | Campos válidos se conservan; errores se señalan con contexto |
| Registro cabe exactamente / se pasa a página nueva / excede tamaño máximo | Sin truncamientos ni registros partidos |
| Importar, cerrar, reabrir y leer páginas | Datos idénticos; error en binario truncado o tamaño incompatible |
| Buffer 2 frames; solicitar `0,0,1,2,0`, liberando cada acceso | **1 hit, 4 misses**, 4 lecturas físicas |
| Buffer 2 frames fijados; solicitar una tercera página | Falla sin reemplazar ninguna hasta que se libere una |
| Hash forzado a un bucket | Inserción, colisiones, actualización, búsqueda y eliminación correctas |
| ID conocido dos veces e ID desconocido | Primero miss, luego hit; ID inexistente no cambia contadores |

**Regla de integración:** si alguien necesita cambiar una firma, estructura o formato, lo comunica al grupo y modifica este documento **antes de fusionar ramas**. Evitar correcciones silenciosas en archivos ajenos.
