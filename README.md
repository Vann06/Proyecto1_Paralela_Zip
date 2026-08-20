# ZipZip Espacial - Proyecto 1 de Computación Paralela

ZipZip Espacial es un screensaver interactivo en vista cenital desarrollado en
C++. La escena representa una granja suspendida en el espacio, con vacas que se
mueven y rebotan dentro del área visible. Detrás de ellas hay un campo de
estrellas que se desplaza lentamente y parpadea con ritmos independientes.

La idea completa incluye un OVNI pilotado por un gato espacial que orbitará la
granja y capturará vacas mediante un rayo tractor, además de planetas, estrellas
y partículas. Primero se construirá una versión secuencial estable y luego se
paralelizarán las actualizaciones independientes con OpenMP para medir tiempos,
FPS, speedup y eficiencia.

## Integrantes

- Vianka Castro
- Ricardo Godinez
- Abby Donis

## Estado actual

- Ventana gráfica de 900 x 700 creada con SDL2.
- Renderizado 3D con OpenGL.
- Carga de modelos Wavefront OBJ.
- Cantidad configurable de vacas; el valor predeterminado es 20.
- Posición, velocidad, giro, escala y color por vaca.
- Movimiento pseudoaleatorio y rebote contra los límites.
- Campo de 180 estrellas con movimiento lento y brillo pulsante.
- HUD con FPS, cantidad de vacas y cantidad de estrellas.
- Pausa, wireframe, face culling y control de VSync.
- Código separado en aplicación, renderizado, simulación y recursos.
- Sistema de compilación con CMake.

El OVNI, los planetas, las partículas y la versión OpenMP todavía no están
implementados.

### Avance 1 completado

| Requisito | Implementación | Estado |
|---|---|---|
| Ventana gráfica y API elegida | `SDL_CreateWindow` y contexto OpenGL en `src/main.cpp` | Completo |
| Variable en memoria | `Escena::vacas`, un `std::vector<Instancia>` definido en `scene.h` | Completo |
| Renderizado de un elemento | `Renderer` dibuja instancias de `assets/models/cow.obj` | Completo |
| Actualización de ubicación | `actualizarEscena` modifica `x` y `y` mediante `vx`, `vy` y `dt` | Completo |

El primer commit que contiene todos los requisitos del Avance 1 es
[`b274724d661c268f01373b89c1058f26751ccef5`](https://github.com/Vann06/Proyecto1_Paralela_Zip/commit/b274724d661c268f01373b89c1058f26751ccef5).

## Arquitectura del repositorio

```text
Proyecto1_Paralela_Zip/
|-- CMakeLists.txt
|-- assets/
|   `-- models/
|       `-- cow.obj
|-- docs/
|   `-- Propuesta Proyecto.pdf
|-- include/
|   `-- zipzip/
|       |-- assets/
|       |   `-- obj_loader.h
|       |-- rendering/
|       |   `-- renderer.h
|       `-- simulation/
|           |-- scene.h
|           `-- starfield.h
|-- src/
|   |-- main.cpp
|   |-- assets/
|   |   `-- obj_loader.cpp
|   |-- rendering/
|   |   `-- renderer.cpp
|   `-- simulation/
|       |-- scene.cpp
|       `-- starfield.cpp
|-- .gitignore
`-- README.md
```

### Responsabilidades

- `src/main.cpp`: inicializa SDL2, procesa eventos y coordina el ciclo principal.
- `simulation/scene`: almacena y actualiza el estado de las vacas.
- `simulation/starfield`: crea y actualiza las estrellas sin depender de OpenGL.
- `rendering/renderer`: contiene todas las llamadas de dibujo OpenGL y el HUD.
- `assets/obj_loader`: transforma un archivo OBJ en arreglos listos para renderizar.
- `assets/models`: contiene los modelos 3D utilizados por la aplicación.

Esta separación permite paralelizar posteriormente las actualizaciones de la
simulación sin ejecutar SDL2 u OpenGL desde los hilos de OpenMP. El renderizado
debe permanecer en el hilo principal.

## Compilar en Windows con MSYS2 UCRT64

### 1. Instalar las herramientas

Abrir **MSYS2 UCRT64** e instalar el compilador, SDL2, CMake y Ninja:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

El prompt de la terminal debe indicar `UCRT64`; no se debe compilar desde
`MSYS2 MSYS`.

### 2. Obtener el repositorio

```bash
git clone https://github.com/Vann06/Proyecto1_Paralela_Zip.git
cd Proyecto1_Paralela_Zip
```

Si ya se descargó, hay que entrar a la ubicación correspondiente. En MSYS2,
`/c/` representa la unidad `C:` de Windows:

```bash
cd /c/ruta/donde/esta/Proyecto1_Paralela_Zip
```

### 3. Compilar con CMake

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

CMake compila el ejecutable y copia automáticamente `assets/` a la carpeta de
salida.

### 4. Ejecutar

```bash
./build/zipzip.exe
```

Para cambiar la cantidad de vacas:

```bash
./build/zipzip.exe 100
```

## Compilación manual en Windows

Si no se desea utilizar CMake, se puede compilar desde la raíz del repositorio
con una sola línea:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/assets/obj_loader.cpp src/rendering/renderer.cpp src/simulation/scene.cpp src/simulation/starfield.cpp -o zipzip.exe -lmingw32 -lSDL2main -lSDL2 -lopengl32
```

Ejecutar desde la raíz para que se encuentre `assets/models/cow.obj`:

```bash
./zipzip.exe
```

En Bash, `\` solo continúa un comando cuando es el último carácter de una
línea. Para evitar errores al copiar, la instrucción anterior se presenta en
una sola línea.

## Compilar en Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build libsdl2-dev libgl1-mesa-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/zipzip
```

En WSL se necesita WSLg o algún otro servidor gráfico para mostrar la ventana.

## Argumentos

Los argumentos pueden proporcionarse en cualquier orden:

```bash
# Cantidad de vacas con el modelo predeterminado
./zipzip.exe 50

# Cantidad de vacas y otro modelo OBJ
./zipzip.exe 50 ruta/al/modelo.obj

# Los mismos argumentos en orden inverso
./zipzip.exe ruta/al/modelo.obj 50
```

- Un argumento numérico indica la cantidad de vacas.
- Un argumento no numérico indica la ruta del modelo OBJ.
- Sin argumentos se crean 20 vacas y se carga `assets/models/cow.obj`.
- Cuando se utiliza CMake, reemplazar `./zipzip.exe` por `./build/zipzip.exe`.

## Controles

| Tecla | Acción |
|---|---|
| `Espacio` | Pausar o continuar toda la simulación |
| `W` | Alternar entre relleno y wireframe |
| `C` | Activar o desactivar face culling |
| `V` | Activar o desactivar VSync |
| `Esc` | Cerrar el programa |

## Cómo funcionan las estrellas

Cada estrella almacena posición, velocidad, tamaño, brillo base, amplitud,
frecuencia y fase. En cada cuadro:

1. Su posición cambia de acuerdo con `vx`, `vy` y `dt`.
2. Si sale de la pantalla, reaparece por el borde opuesto.
3. Su brillo se calcula con una función seno.
4. El renderer las dibuja antes que las vacas para mantenerlas en el fondo.

Las fases y frecuencias son diferentes, por lo que las estrellas no parpadean
todas al mismo tiempo. Su actualización también es independiente y podrá
paralelizarse más adelante.

## Próximos pasos

- Dibujar el terreno y los límites visuales de la granja.
- Incorporar el OVNI y su trayectoria orbital.
- Seleccionar una vaca y capturarla con un rayo tractor.
- Añadir partículas para el rayo y las explosiones de estrellas.
- Incorporar planetas decorativos en el fondo.
- Mantener y medir una versión secuencial de referencia.
- Paralelizar vacas, estrellas y partículas con OpenMP.
- Registrar FPS, tiempos, speedup y eficiencia.
- Agregar pruebas automáticas para simulación y carga de modelos.
