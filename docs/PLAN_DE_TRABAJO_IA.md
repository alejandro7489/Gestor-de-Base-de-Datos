# Plan de desarrollo colaborativo y contexto para IA

> [!IMPORTANT]
> **DOCUMENTO PRINCIPAL PARA EL EQUIPO Y CUALQUIER ASISTENTE DE IA.** Leer completo antes de programar o pedir cambios. Es la guía de reparto y alcance; los detalles binarios y de API están en [contratos.md](contratos.md). El trabajo debe partir del **scaffold existente**, nunca de un proyecto nuevo.

**Proyecto:** Spotify Database Manager · **Lenguaje:** C++17 · **Curso:** Estructura de Datos (4.º semestre)  
**Equipo:** Esteban Villagarcia, Fernando Lizarraga y Leonardo Cuadros.  
**Acuerdo:** 21 de noviembre de 2026 · **Demostración prevista:** lunes 23 de noviembre de 2026.  
**Repositorio:** https://github.com/alejandro7489/Gestor-de-Base-de-Datos

## 1. Contexto y objetivo de esta fase

Construiremos un motor académico de almacenamiento musical, con datos provenientes de un CSV de Spotify (versión reducida con 16 columnas). El motor debe poder administrar **muchos registros sin cargar todas las canciones en RAM**.

El recorrido real que queremos demostrar es:

```text
CSV -> LectorCSV -> serialización y páginas -> archivo binario persistente
                                                      |
Consulta / solicitud -> BufferPool (frames en RAM) ---+
                            |              |
                          HIT          MISS -> lectura de página en disco
```

**Meta esencial del lunes:** importar canciones, recuperar páginas reales, realizar una solicitud que cause **MISS** (carga desde disco) y repetirla para obtener **HIT** (se reutiliza RAM). Se deben poder mostrar los frames ocupados, estadísticas y lecturas físicas. Además, preparar pruebas de índice Hash e integración si el tiempo lo permite.

**No es la meta del lunes:** LRU funcional, CLOCK/MRU, AVL, Heap, SQL completo, reproducción musical, interfaz nueva, rankings, filtros y CRUD completo. `ListaLRU.h/.cpp` queda sin implementar; la política temporal de reemplazo es determinista y sencilla. La `InterfazConsola` existente es una demo con datos ilustrativos, no el motor.

## 2. Estado real del código: NO crear otro scaffold

El repositorio ya contiene `include/`, `src/`, `docs/arquitectura.md`, `README.md`, `CMakeLists.txt` y `.gitignore`. Existen `Cancion`, `Pagina`, `Frame`, `LectorCSV`, `Almacenamiento`, `TablaHash`, `ListaLRU`, `BufferPool`, `GestorMusical`, `InterfazConsola` y `main.cpp`.

- `Cancion` modela 16 atributos de Spotify; `Pagina` tiene ID y un vector de bytes.
- `LectorCSV::leerSiguiente()`, las operaciones paginadas de `Almacenamiento`, `BufferPool::obtenerPagina()`, `TablaHash` y las consultas musicales están sin implementar.
- `BufferPool` ya crea un array de `Frame`, pero sus hits/misses no son reales todavía.
- El archivo CSV y el binario **no están versionados** por `.gitignore`; cada persona debe colocarlos localmente.

Antes de editar, inspeccionar el estado más reciente del repositorio. No sobrescribir código funcional o decisiones nuevas del equipo.

## 3. Reparto exacto y límites de edición

| Persona | Rama propia | Archivos que puede implementar |
|---|---|---|
| **Fernando Lizarraga** | `feature/storage` | `include/LectorCSV.h`, `src/LectorCSV.cpp`, `include/Pagina.h`, `include/Almacenamiento.h`, `src/Almacenamiento.cpp` |
| **Leonardo Cuadros** | `feature/buffer` | `include/Frame.h`, `include/BufferPool.h`, `src/BufferPool.cpp` |
| **Esteban Villagarcia** | `feature/integration` | `include/TablaHash.h`, `src/TablaHash.cpp`, `include/GestorMusical.h`, `src/GestorMusical.cpp`, `src/main.cpp`, `tests/` |

**Archivos compartidos**: `CMakeLists.txt`, `README.md`, `docs/`, `AGENTS.md` y la consola solo cambian con coordinación. La conexión final con `InterfazConsola.h/.cpp` corresponde a Esteban si sobra tiempo, sin rehacer su diseño. No modificar `ListaLRU` en esta fase.

### A. Fernando — Storage

**Responsabilidad:** conseguir que canciones reales puedan convertirse en páginas y que esas páginas se lean desde un archivo binario.

Tareas:
1. Implementar lectura incremental y segura del CSV, validando las 16 columnas.
2. Calcular el **tamaño serializado** de cada canción (no `sizeof(Cancion)`).
3. Serializar y deserializar cada campo con el esquema acordado; textos UTF-8 y opcionales presentes/ausentes.
4. Empaquetar registros completos en páginas de tamaño configurable, controlando cabeceras y registro que no cabe.
5. Implementar archivo binario `SDB1`, `crear()`, `abrir()`, `cantidadPaginas()`, `leerPagina()`, `escribirPagina()`.
6. Probar importar, cerrar, reabrir y obtener las mismas canciones. Probar tamaños de página distintos y casos de error.

**Definición de terminado:** muestra CSV -> páginas reales -> archivo binario -> lectura de páginas -> canciones idénticas. Ningún registro truncado ni archivo sobrescrito accidentalmente.

**No modificar:** BufferPool, Frame, TablaHash, GestorMusical, `main.cpp`, la consola o el README.

### B. Leonardo — Buffer Manager

**Responsabilidad:** administrar RAM limitada, usar páginas reales del disco e informar hits/misses.

Tareas:
1. Extender `Frame` con `pinCount` y `dirtyBit`, conservando su `Pagina` y `ocupado`.
2. Implementar búsqueda de página entre frames (recorrido lineal permitido por ahora).
3. Si página está cargada: hit, reutilizarla, `pinCount++`; **cero lectura de disco**.
4. Si no está: miss, obtener un frame libre o el primero limpio y sin pin; leer realmente la página desde `Almacenamiento`.
5. Crear `liberarPagina()` y validar que un frame fijado nunca sea reemplazado.
6. Mantener estadísticas verificables de hits, misses y opcionalmente lecturas de disco.

**Definición de terminado:** usando dos frames, la secuencia `0,0,1,2,0`, liberando cada solicitud, da **1 hit y 4 misses**; con los dos frames fijados simultáneamente, solicitar una tercera página falla sin corromper nada.

**No implementar LRU** y no modificar CSV, almacenamiento, Hash, GestorMusical, `main.cpp`, consola o documentación compartida.

### C. Esteban — Hash, integración y pruebas

**Responsabilidad:** unir el motor, implementar un índice propio y verificar que los datos sean reales.

Tareas:
1. Implementar `TablaHash` propia: insertar, buscar, eliminar, manejar colisiones y claves repetidas.
2. Preparar pruebas separadas de Hash (por ejemplo, un único bucket para forzar colisiones).
3. Coordinar `GestorMusical::importarCSV()`, asociando `track_id -> pageId` sin cargar todas las canciones en RAM.
4. Al reabrir, reconstruir el índice leyendo las páginas binarias **sin calentar previamente el BufferPool**.
5. Integrar búsqueda `track_id -> pageId -> BufferPool -> Cancion`; liberar el pin aunque ocurra una excepción.
6. Crear el driver `tests/monday.cpp` para comprobar Storage, Buffer e integración con páginas reales; definir cómo se compila y ejecuta el test.
7. Conectar la consola existente solo si las pruebas técnicas funcionan y sobra tiempo.

**Definición de terminado:** Hash correcto; canción conocida se obtiene desde el archivo real; segunda consulta al mismo ID da hit; ID desconocido devuelve ausencia sin aumentar contadores.

**No modificar:** almacenamiento, Pagina, LectorCSV, Frame, BufferPool o ListaLRU sin acuerdo.

## 4. Contratos obligatorios

**Fuente de verdad técnica:** [docs/contratos.md](contratos.md). Los tres deben aprobarlo antes de cambiar headers públicos.

Resumen de decisiones:

- Página fija de **4096 bytes** por defecto; capacidad de frames independiente.
- Archivo: 8 bytes de cabecera (`SDB1` + tamaño de página `uint32 LE`); página `p` empieza en `8 + p * pageSize`.
- Página: cantidad de registros `uint32 LE`, luego parejas de longitud del registro y bytes; padding de ceros.
- Registro: 16 campos en orden, strings con longitud, opcionales con byte de presencia y números de ancho fijo.
- `obtenerPagina` fija la página; `liberarPagina` la libera. Nunca reemplazar `pinCount > 0`.
- Sin LRU, seleccionar el **primer frame limpio y no fijado** si no quedan frames libres.
- Hash de canciones: `track_id → pageId`; revisar `track_id` dentro de la página para obtener la canción.
- Fallos no se ocultan ni se cuentan como lecturas exitosas.

Si una interfaz actual no coincide con el documento, comunicar el conflicto antes de cambiarla. **No corregir unilateralmente archivos ajenos.**

## 5. Orden para evitar conflictos

1. Aprobar `docs/contratos.md` y este documento; incorporar solo la documentación común a `main`.
2. Cada integrante hace `git pull --ff-only` y crea **su rama desde el mismo `main` actualizado**.
3. Trabajo paralelo por propietario, sin tocar otras clases. Commits pequeños y explicables.
4. Probar cada módulo de forma aislada. Fernando hace round-trip CSV/binario; Leonardo prueba páginas conocidas; Esteban prueba Hash.
5. Integrar **Storage + Buffer primero**: página real -> miss -> hit. Luego índice/gestor, y finalmente consola si es posible.
6. Abrir Pull Requests y revisar con los demás; no fusionar cambios incompatible con `contratos.md`.

```bash
git switch main
git pull --ff-only
git switch -c feature/storage   # Fernando
# feature/buffer (Leonardo) o feature/integration (Esteban)
```

No usar `git reset --hard`, force push ni sobrescrituras; no realizar pushes o merges automáticos desde una IA sin que el responsable los autorice. Compartir un método nuevo solo después de revisar su contrato.

## 6. Pruebas que deben pasar

| Escenario | Esperado |
|---|---|
| CSV con comillas, comas, líneas internas, opcionales vacíos y enteros `123.0` | Datos recuperados correctamente |
| Registros de diferentes tamaños | Páginas completas sin registros cortados; error cuando una canción es demasiado grande |
| Archivo binario abierto de nuevo | Mismas canciones; tamaño incompatible o archivo truncado produce error |
| Buffer con dos frames: `0,0,1,2,0` liberando después de cada acceso | **1 hit, 4 misses, 4 lecturas físicas** |
| Ambos frames fijados al solicitar tercera página | Fallo controlado, nada sobrescrito |
| Hash colisiones forzadas | Insertar/buscar/actualizar/eliminar correctamente |
| Mismo ID conocido consultado dos veces | Primer acceso miss, segundo hit, sin nueva lectura de disco |
| ID desconocido | No altera estadísticas de buffer |

Los resultados de ejemplo impresos por `InterfazConsola.cpp` no cuentan como pruebas. **No basta con que compile**: debemos poder recuperar bytes reales y justificar cada contador.

## 7. Instrucciones obligatorias para cualquier IA

1. Leer **primero** `AGENTS.md`, este plan, `docs/contratos.md`, `docs/arquitectura.md`, README y los archivos del módulo propio.
2. Saber qué integrante solicita el trabajo y limitar las modificaciones a los archivos de su sección. Si no está claro el responsable, no modificar código compartido.
3. No regenerar scaffold, cambiar firmas o formato binario sin autorización, ni completar funciones de otro integrante.
4. C++17 simple y comprensible por estudiantes de cuarto semestre; evita frameworks, patrones empresariales y abstracciones opacas.
5. No reemplazar el Hash académico con `std::unordered_map` ni implementar LRU de forma adelantada.
6. Mantener CSV originales, archivos binarios y datos locales sin modificaciones innecesarias. Nunca versionar datasets grandes.
7. No inventar ejecución exitosa, métricas, resultados de consultas ni pruebas. Indicar qué se ejecutó realmente.
8. Informar al final: archivos tocados, qué se hizo, cómo probar, resultado real y qué queda pendiente.
9. No hacer `commit`, `push`, `merge` ni editar archivos ajenos sin permiso expreso del integrante.
10. Si detectas contradicción entre código y contrato, **reportarla**, no resolverla silenciosamente.

### Prompt para IA de Fernando

> Estoy trabajando como Fernando en `feature/storage`. Lee completos `AGENTS.md`, `docs/PLAN_DE_TRABAJO_IA.md` y `docs/contratos.md` y examina mis archivos actuales. Implementa únicamente `LectorCSV`, `Pagina` y `Almacenamiento`, con páginas binarias reales y serialización segura. Prueba round-trip y tamaños configurables. No toques módulos de Leonardo/Esteban, CMake, README o la consola. No hagas commits automáticos. Explica cada decisión para poder defenderla en clase.

### Prompt para IA de Leonardo

> Estoy trabajando como Leonardo en `feature/buffer`. Lee completos `AGENTS.md`, `docs/PLAN_DE_TRABAJO_IA.md` y `docs/contratos.md` y examina mis archivos actuales. Implementa únicamente `Frame` y `BufferPool`, con pin/unpin, hit/miss, lectura real de páginas, contadores y sustitución temporal del primer frame limpio y no fijado. No LRU. Prueba la secuencia `0,0,1,2,0` con dos frames. No toques Storage, Hash, Gestor, CMake ni la consola. No hagas commits automáticos.

### Prompt para IA de Esteban

> Estoy trabajando como Esteban en `feature/integration`. Lee completos `AGENTS.md`, `docs/PLAN_DE_TRABAJO_IA.md` y `docs/contratos.md` y examina mis archivos actuales. Implementa nuestra `TablaHash` propia y pruebas; luego integra `GestorMusical`, reconstrucción del índice en reapertura y `tests/monday.cpp`. No toques módulos de Fernando o Leonardo ni reescribas la interfaz. Demuestra un miss real seguido de un hit; no hagas commits automáticos.

## 8. Definición de terminado y siguientes fases

Un módulo está terminado solo si **compila, pasa sus pruebas, maneja errores, no inventa resultados, respeta contratos y su autor puede explicarlo**. La funcionalidad completa podrá integrarse después de la demostración del lunes. Las mejoras futuras incluyen tabla hash de páginas residente, lista doblemente enlazada LRU, estadísticas avanzadas, consultas y UI conectada a datos reales.

**Prioridad: almacenamiento verdadero + buffer real. Simplicidad y comprensión antes que complejidad.**
