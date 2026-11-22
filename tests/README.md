# Pruebas del motor musical

Ejecuta los comandos desde la raíz de `spotify_dbms`. Cada prueba tiene su propio `main`.

## Storage: CSV, páginas y archivo binario

Esta prueba crea entradas y binarios pequeños en una carpeta temporal del sistema; no
usa ni altera los datasets locales.

```powershell
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /Iinclude /Fobuild\ /Febuild\storage.exe tests/storage.cpp src/LectorCSV.cpp src/Pagina.cpp src/Almacenamiento.cpp
.\build\storage.exe
```

Con GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tests/storage.cpp src/LectorCSV.cpp src/Pagina.cpp src/Almacenamiento.cpp -o build/storage
./build/storage
```

## Tabla Hash

En PowerShell para desarrolladores de Visual Studio:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /Iinclude /Fobuild\ /Febuild\tabla_hash.exe tests/tabla_hash.cpp src/TablaHash.cpp
.\build\tabla_hash.exe
```

Con GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tests/tabla_hash.cpp src/TablaHash.cpp -o build/tabla_hash
./build/tabla_hash
```

Un solo bucket obliga a las cuatro claves de la prueba a compartir la misma cadena.
También se comprueba que las copias tengan sus propios nodos y que la tabla pueda
reutilizar un bucket después de vaciarlo.

## Tabla Hash con los IDs del CSV local

Esta prueba enlaza únicamente `TablaHash.cpp`. Usa el índice del registro
como valor temporal y comprueba los 1000 IDs antes y después de modificar la tabla.
Los tiempos solo cubren los recorridos de inserción y búsqueda.

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 /Iinclude /Fobuild\ /Febuild\hash_dataset.exe tests/hash_dataset.cpp src/TablaHash.cpp
.\build\hash_dataset.exe ..\datasets\spotify_sample_1000.csv
```

## Integración con el CSV real

La prueba importa a un binario temporal con páginas de 2048 bytes. Cierra el primer
gestor y crea otro con una ruta CSV inexistente para comprobar que recupera la canción
desde el binario. Compara el primer registro del CSV con la consulta y exige que
los contadores pasen por `(0, 0)`, `(0, 1)`, `(1, 1)` y `(1, 1)` en orden
`(hits, misses)`. El último par corresponde a la consulta de un ID inexistente.

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /Iinclude /Fobuild\ /Febuild\monday.exe tests/monday.cpp src/GestorMusical.cpp src/TablaHash.cpp src/LectorCSV.cpp src/Almacenamiento.cpp src/BufferPool.cpp src/ListaLRU.cpp
.\build\monday.exe ..\datasets\spotify_sample_1000.csv
```

Si Storage mueve la implementación de `Pagina` a un `.cpp`, inclúyelo al compilar.
La prueba necesita la importación y búsqueda de `GestorMusical`, la lectura CSV,
la serialización y el acceso paginado de Storage, y la carga y liberación de páginas
del Buffer Pool. En el scaffold se detiene al iniciar la importación.

Para verificar también el acceso a disco de forma independiente, queda pendiente
comprobar el contador de lecturas físicas si el Buffer Pool lo expone. Con el
contrato actual también se puede hacer una prueba directa: cargar y liberar una
página real, cerrar el almacenamiento y volver a pedir la página residente.
La segunda solicitud debe ser un hit con el archivo cerrado.
