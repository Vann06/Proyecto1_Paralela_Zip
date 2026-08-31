# ZipZip - Proyecto 1 de Computacion Paralela

ZipZip es un screensaver interactivo en vista cenital desarrollado en C++.
La escena representa una granja suspendida en el espacio con varias vacas en
movimiento. Cuando un dia llego un OVNI pilotado por un gato espacial, listo
para capturar a las vacas. Se ven las estrellas, planetas y particulas para
representar la captura de las vacas.

El proyecto comienza con una version secuencial que sirve como referencia
funcional y de rendimiento. Las actualizaciones independientes de los
elementos se paralelizan con OpenMP para comparar tiempos, FPS, speedup y
eficiencia con diferentes cantidades de elementos e hilos.

## Integrantes

- Vianka Castro
- Ricardo Godinez
- Abby Donis

## Estado actual

Avance 1:

- Ventana grafica de 900 x 700 creada con SDL2 y renderizado 3D con OpenGL.
- Carga de modelos Wavefront OBJ (vaca, OVNI, planeta).
- Cantidad configurable de vacas y estrellas.
- Posicion, velocidad, giro, escala y color por vaca.
- Rebote vectorial en la plataforma (la formula de reflexion viene de
  `docs/matematica_rebote_rombo.md`; la plataforma actual es una cupula
  semicircular que nace del borde inferior de la pantalla, como el
  horizonte de un planeta).
- Cuatro planetas con superficies procedurales distintas, rotacion propia y
  dos con aros.
- Campo de estrellas y estrellas fugaces animadas.
- HUD con FPS y cantidad de vacas.

Avance 2:

- OVNI que sobrevuela la plataforma en una trayectoria de Lissajous; las
  vacas se separan entre si y se mantienen agrupadas por una fuerza de
  cohesion (ya no huyen del OVNI).
- Simulacion paralelizada con OpenMP: la interaccion entre vacas es O(N^2) e
  intencionalmente compute-bound (candidato real a escalar con los hilos);
  la integracion del movimiento es O(N) y memory-bound. El contraste entre
  ambas es a proposito, para comparar en el informe.
- Modo `--bench` sin ventana para medir con N grande sin que el render sea
  el cuello de botella, mas `scripts/benchmark.sh` para generar el CSV del
  informe (speedup, eficiencia, schedule, false sharing).
- `--dump-estado` para verificar que una corrida serial y una paralela con
  la misma semilla producen exactamente el mismo resultado.
- HUD ampliado con tiempo de simulacion, hilos activos y distancia minima
  al OVNI.

## Estructura del repositorio

```text
Proyecto1_Paralela_Zip/
|-- CMakeLists.txt
|-- assets/
|   `-- models/            cow.obj, ufo_gato.obj, planet.obj
|-- include/zipzip/
|   |-- core/               camara.h, cronometro.h, rng.h (compartidos)
|   |-- assets/             obj_loader.h
|   |-- simulation/         scene.h, starfield.h
|   `-- rendering/          renderer.h
|-- src/
|   |-- main.cpp            ventana, eventos, ciclo principal, modo --bench
|   |-- assets/obj_loader.cpp
|   |-- simulation/scene.cpp      vacas, OVNI, planetas
|   |-- simulation/starfield.cpp  estrellas y estrellas fugaces
|   `-- rendering/renderer.cpp
|-- scripts/
|   `-- benchmark.sh        barrido de --bench a CSV
|-- docs/
|   |-- matematica_rebote_rombo.md
|   `-- Propuesta Proyecto.pdf
`-- README.md
```

## Compilar

El proyecto usa CMake (>= 3.20) y requiere SDL2, OpenGL y, opcionalmente,
OpenMP.

### Linux

```bash
# Dependencias (ejemplo en distros basadas en Arch/pacman):
#   sudo pacman -S cmake sdl2 mesa
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/zipzip
```

`CMAKE_BUILD_TYPE` se fija en `Release` automaticamente si no se especifica
otro, para que nadie compare tiempos de una build sin optimizar por
accidente.

### Windows con MSYS2

Abrir la terminal **MSYS2 UCRT64** e instalar el compilador, CMake y SDL2:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
                    mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-ninja
```

Luego, dentro del repositorio:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/zipzip.exe
```

### Compilar sin OpenMP (referencia serial)

```bash
cmake -S . -B build-serial -DZIPZIP_OPENMP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-serial -j
```

Sirve como version de referencia para comparar contra la build con OpenMP
(`ZIPZIP_OPENMP=ON`, que es el valor por defecto).

## Uso

```bash
./build/zipzip [N] [opciones]
```

| Opcion | Default | Descripcion |
|---|---|---|
| `N` (posicional) | 20 | Forma corta de `--vacas N`, por compatibilidad |
| `--vacas N` | 20 | Cantidad de vacas |
| `--estrellas N` | 180 | Cantidad de estrellas de fondo |
| `--modelo ruta` | `assets/models/cow.obj` | Modelo OBJ de la vaca |
| `--hilos N` | automatico | Hilos de OpenMP (`omp_set_num_threads`) |
| `--schedule tipo` | `static` | `static`, `dynamic` o `guided` |
| `--chunk N` | automatico | Tamano de bloque del schedule (ver false sharing abajo) |
| `--bench` | apagado | Corre sin ventana y mide en vez de dibujar (ver abajo) |
| `--frames N` | 20 (bench) | Cuadros medidos en modo `--bench` |
| `--dump-estado` | apagado | Imprime posicion/velocidad final de cada vaca en vez de un CSV |

## Controles

Sin controles de simulacion: la ventana corre siempre a maxima velocidad, sin
VSync (no se limita a la frecuencia del monitor), justamente para poder ver
el efecto de la paralelizacion en los FPS. La unica tecla es `Esc`, para
cerrar el programa (la 'x' de la ventana tambien funciona).

El HUD muestra el tiempo de simulacion (`SIM`) por separado del tiempo total
de frame: en esa parte es donde actua OpenMP, mientras que el dibujado
(dominado hoy por el numero de vacas visibles) no se paraleliza.

## Modo benchmark (`--bench`)

Corre la simulacion `--frames` veces con un `dt` fijo (no el reloj real) y
sin inicializar SDL ni OpenGL, para poder subir `--vacas` mucho mas alla de
lo que el render podria dibujar y comparar tiempos de forma reproducible.
Escribe una fila CSV a stdout (el encabezado va a stderr, para poder
concatenar varias corridas sin repetirlo):

```bash
./build/zipzip --bench --vacas 5000 --hilos 8 --schedule dynamic
```

```text
vacas,estrellas,hilos,schedule,chunk,ms_sim_avg,ms_sim_p95,ms_pasada_a_avg,ms_pasada_b_avg,ms_render_avg,fps
5000,180,8,dynamic,0,12.340000,12.900000,12.100000,0.240000,0.000000,81.037
```

`ms_pasada_a` es la interaccion O(N^2) entre vacas (compute-bound, donde se
espera el speedup real); `ms_pasada_b` es la integracion O(N)
(memory-bound, donde vive el efecto de false sharing con
`--schedule static --chunk 1`, ya que `Instancia` mide 40 bytes y la linea
de cache son 64).

Para generar el CSV completo del informe (speedup/eficiencia, comparacion
de schedules y el experimento de false sharing):

```bash
scripts/benchmark.sh build/zipzip > resultados.csv
```

Variables de entorno opcionales: `VACAS_LIST`, `HILOS_LIST`, `FRAMES`,
`VACAS_CHUNK`, `VACAS_SCHED` (ver los comentarios al inicio del script).

### Verificar la paralelizacion

`--dump-estado` imprime la posicion/velocidad final de cada vaca. Con la
misma semilla, una corrida serial y una paralela deben coincidir byte a
byte:

```bash
./build/zipzip --bench --vacas 3000 --frames 30 --hilos 1  --dump-estado > serial.txt
./build/zipzip --bench --vacas 3000 --frames 30 --hilos 8  --dump-estado > paralelo.txt
diff serial.txt paralelo.txt   # sin diferencias esperado
```

Esto funciona porque cada vaca solo lee el estado de las demas y escribe
unicamente en su propia posicion/aceleracion: no hay condiciones de carrera
ni dependencia del orden de ejecucion entre hilos.

## Arquitectura

- **Aplicacion** (`src/main.cpp`): ventana, eventos, ciclo principal y modo
  `--bench`.
- **Simulacion** (`src/simulation/`): vacas, OVNI, planetas, estrellas y las
  reglas de movimiento. No depende de SDL ni de OpenGL.
- **Renderizado** (`src/rendering/`): dibujo con OpenGL y HUD.
- **Recursos** (`src/assets/`): carga de modelos OBJ.
- **Nucleo compartido** (`include/zipzip/core/`): RNG determinista,
  parametros de camara y cronometro, usados tanto por la simulacion como
  por el modo `--bench`.
