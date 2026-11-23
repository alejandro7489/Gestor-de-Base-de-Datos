# Pruebas del motor musical

Ejecuta los comandos desde la raíz de `spotify_dbms`. Cada prueba tiene su propio `main`.

## Storage: CSV, páginas y archivo binario

Esta prueba crea entradas y binarios pequeños en una carpeta temporal del sistema; no
usa ni altera los datasets locales.

```powershell
New-Item -ItemType Directory -Force build | Out-Null
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

La prueba importa las 1000 canciones a un binario temporal con páginas de 4096
bytes y un Buffer Pool de dos frames. Cierra el primer gestor y abre el binario
con otro gestor cuya ruta CSV no existe. Verifica la cantidad de registros y
compara los 16 campos de cada canción, incluidos los opcionales, tanto en las
páginas como en las consultas por Hash. También prueba IDs duplicados, el
rechazo de registros demasiado grandes y que una importación nueva preserve
un binario existente. Se verifican las búsquedas parciales por nombre, artista
y álbum y que sus lecturas directas dejen intactos los contadores del buffer.

```powershell
New-Item -ItemType Directory -Force build | Out-Null
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /Iinclude /Fobuild\ /Febuild\monday.exe tests/monday.cpp src/GestorMusical.cpp src/TablaHash.cpp src/LectorCSV.cpp src/Almacenamiento.cpp src/Pagina.cpp src/BufferPool.cpp src/ListaLRU.cpp
.\build\monday.exe ..\datasets\spotify_sample_1000.csv
```

Con GCC:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude tests/monday.cpp src/GestorMusical.cpp src/TablaHash.cpp src/LectorCSV.cpp src/Almacenamiento.cpp src/Pagina.cpp src/BufferPool.cpp src/ListaLRU.cpp -o build/monday
./build/monday ../datasets/spotify_sample_1000.csv
```

La consulta inicial produce un MISS y una lectura de página. La repetición
produce un HIT y el ID inexistente deja intactos los contadores. En la prueba
independiente con dos frames, la secuencia `0,0,1,2,0` produce un HIT y cuatro
MISS. También se verifica que los pines impidan el reemplazo y que pedir una
página residente con el archivo cerrado produzca un HIT.
En una prueba con un solo frame, se comprueba que un error de decodificación
libere el pin para que la siguiente página pueda ocupar el frame.

`lecturasDisco` cuenta llamadas exitosas del Buffer Pool a
`Almacenamiento::leerPagina`. Las lecturas directas de reconstrucción quedan
fuera de ese contador. La caché del sistema operativo puede responder a una
lectura, por lo que esta métrica no mide accesos físicos al dispositivo.
