cmake_minimum_required(VERSION 3.16)
project(SpotifyDatabaseManager VERSION 0.1 LANGUAGES CXX)

add_executable(spotify_dbms
    src/main.cpp
    src/LectorCSV.cpp
    src/Almacenamiento.cpp
    src/Pagina.cpp
    src/TablaHash.cpp
    src/ListaLRU.cpp
    src/BufferPool.cpp
    src/GestorMusical.cpp
    src/InterfazConsola.cpp
)

target_include_directories(spotify_dbms PRIVATE include)
set_target_properties(spotify_dbms PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(spotify_dbms PRIVATE /W4 /utf-8)
else()
    target_compile_options(spotify_dbms PRIVATE -Wall -Wextra -Wpedantic)
endif()
