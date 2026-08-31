cmake_minimum_required(VERSION 3.20)

project(ZipZipEspacial VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sin esto, un build "limpio" (sin -DCMAKE_BUILD_TYPE=Release explicito) queda
# sin optimizaciones y todas las mediciones de tiempo/FPS quedan invalidas.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(ZIPZIP_OPENMP "Compilar con soporte OpenMP" ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

if(ZIPZIP_OPENMP)
    find_package(OpenMP)
    if(NOT OpenMP_CXX_FOUND)
        message(WARNING "ZIPZIP_OPENMP esta activo pero no se encontro OpenMP; "
                         "se compila la version serial.")
        set(ZIPZIP_OPENMP OFF)
    endif()
endif()

add_executable(zipzip
    src/main.cpp
    src/assets/obj_loader.cpp
    src/rendering/renderer.cpp
    src/simulation/scene.cpp
    src/simulation/starfield.cpp
)

target_include_directories(zipzip PRIVATE include)

if(TARGET SDL2::SDL2main)
    target_link_libraries(zipzip PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(zipzip PRIVATE SDL2::SDL2 OpenGL::GL)

if(ZIPZIP_OPENMP)
    target_link_libraries(zipzip PRIVATE OpenMP::OpenMP_CXX)
endif()

if(MSVC)
    target_compile_options(zipzip PRIVATE /W4)
else()
    target_compile_options(zipzip PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Permite ejecutar el programa desde la carpeta de build sin copiar recursos
# manualmente. La ruta relativa assets/models/cow.obj se conserva.
add_custom_command(TARGET zipzip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:zipzip>/assets"
    COMMENT "Copiando recursos del proyecto"
)
