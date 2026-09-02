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
- El OBJ del OVNI conserva grupos de geometria: nave con acabado metalico y
  gato con emision verde neon.
- Simulacion paralelizada con OpenMP: la interaccion entre vacas es O(N^2) e
  intencionalmente compute-bound (candidato real a escalar con los hilos);
  la integracion del movimiento es O(N) y memory-bound. El contraste entre
  ambas es a proposito, para comparar en el informe.
- Modo `--bench` sin ventana para medir con N grande sin que el render sea
  el cuello de botella, mas `scripts/benchmark.sh` para generar el CSV del
  informe (speedup, eficiencia, schedule, false sharing).
- `--dump-estado` para verificar que una corrida serial y una paralela con
  la misma semilla producen exactamente el mismo resultado.
- Un mismo ejecutable permite seleccionar `--modo serial` o
  `--modo paralelo`; no hay dos copias del proyecto.
- Programacion defensiva para rangos numericos, opciones desconocidas,
  dimensiones minimas del canvas y builds sin OpenMP.
- Las estrellas se actualizan en serial por defecto. La opcion
  `--estrellas-paralelas` existe para medirlas por separado: con las 180
  estrellas habituales el overhead de OpenMP suele ser mayor que el ahorro.
- HUD ampliado con tiempo de simulacion, hilos activos y distancia minima
  al OVNI.

## Estructura del repositorio

```text
Proyecto1_Paralela_Zip/
|-- CMakeLists.txt
|-- assets/
|   |-- models/            cow.obj, ufo_gato.obj, planet.obj
|   `-- textures/          space_background.bmp, grass.bmp
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
|   |-- benchmark.sh        10 mediciones por configuracion a CSV
|   `-- resumir_benchmark.py promedios, speedup y eficiencia
|-- docs/
|   |-- diagrama_flujo_final.md
|   |-- imprescindibles.md  extracto de los 5 diagramas centrales
|   |-- anexo_02_catalogo_funciones.md
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
| `--modo serial\|paralelo` | paralelo* | Activa o desactiva los pragmas OpenMP en el mismo ejecutable |
| `--vacas N` | 20 | Cantidad de vacas |
| `--estrellas N` | 180 | Cantidad de estrellas de fondo |
| `--estrellas-paralelas` | apagado | Paraleliza estrellas para evaluar si compensa el overhead |
| `--modelo ruta` | `assets/models/cow.obj` | Modelo OBJ de la vaca |
| `--hilos N` | automatico | Hilos de OpenMP (`omp_set_num_threads`) |
| `--schedule tipo` | `static` | `static`, `dynamic` o `guided` |
| `--chunk N` | automatico | Tamano de bloque del schedule (ver false sharing abajo) |
| `--ancho N` | 900 | Ancho del canvas; minimo 640 |
| `--alto N` | 700 | Alto del canvas; minimo 480 |
| `--semilla N` | 1234 | Semilla reproducible mayor que cero |
| `--bench` | apagado | Corre sin ventana y mide en vez de dibujar (ver abajo) |
| `--frames N` | 300 | Cuadros medidos en modo `--bench` |
| `--dump-estado` | apagado | Imprime posicion/velocidad final de cada vaca en vez de un CSV |
| `--ayuda` | - | Lista opciones y rangos validos |

\* En una build creada con `ZIPZIP_OPENMP=OFF`, el default es serial. Pedir
`--modo paralelo` en esa build devuelve un error explicativo.

Ejemplos comparables con el mismo binario:

```bash
./build/zipzip --bench --modo serial   --vacas 5000 --frames 30
./build/zipzip --bench --modo paralelo --vacas 5000 --frames 30 --hilos 8
```

## Controles

Sin controles de simulacion: la ventana corre siempre a maxima velocidad, sin
VSync (no se limita a la frecuencia del monitor), justamente para poder ver
el efecto de la paralelizacion en los FPS. La unica tecla es `Esc`, para
cerrar el programa (la 'x' de la ventana tambien funciona).

El HUD muestra el tiempo de simulacion (`SIM`) por separado del tiempo total
de frame: en esa parte es donde actua OpenMP, mientras que el dibujado
(dominado hoy por el numero de vacas visibles) no se paraleliza.

## Personalizar el fondo y la grama

El renderizador carga dos imágenes BMP mediante SDL2, sin depender de
`SDL_image`:

- `assets/textures/space_background.bmp`: fondo que cubre la ventana; las
  estrellas animadas se dibujan encima.
- `assets/textures/grass.bmp`: textura repetible de la cara superior de la
  plataforma.

Se pueden sustituir por otros archivos conservando exactamente esos nombres.
Se recomienda BMP de 24 o 32 bits, fondo con proporción cercana a la ventana
y grama cuadrada sin costuras. Si un archivo falta o no se puede leer, el
programa muestra un aviso y continúa con el color sólido anterior.

## Modo benchmark (`--bench`)

Corre la simulacion `--frames` veces con un `dt` fijo (no el reloj real) y
sin inicializar SDL ni OpenGL, para poder subir `--vacas` mucho mas alla de
lo que el render podria dibujar y comparar tiempos de forma reproducible.
Escribe una fila CSV a stdout (el encabezado va a stderr, para poder
concatenar varias corridas sin repetirlo):

```bash
./build/zipzip --bench --modo paralelo --vacas 5000 --hilos 8 --schedule dynamic
```

```text
modo,vacas,estrellas,hilos,estrellas_paralelas,schedule,chunk,semilla,ms_sim_avg,ms_sim_p95,ms_pasada_a_avg,ms_pasada_b_avg,ms_render_avg,fps
paralelo,5000,180,8,0,dynamic,0,1234,12.340000,12.900000,12.100000,0.240000,0.000000,81.037
```

`ms_pasada_a` es la interaccion O(N^2) entre vacas (compute-bound, donde se
espera el speedup real); `ms_pasada_b` es la integracion O(N)
(memory-bound, donde vive el efecto de false sharing con
`--schedule static --chunk 1`, ya que `Instancia` mide 40 bytes y la linea
de cache son 64).

Para generar el CSV completo del informe, con un minimo de 10 mediciones por
configuracion, y luego calcular promedios, speedup y eficiencia:

```bash
bash scripts/benchmark.sh build/zipzip > resultados.csv
python scripts/resumir_benchmark.py resultados.csv > resumen.csv
```

El resumen usa las formulas:

```text
speedup    = tiempo_serial_promedio / tiempo_paralelo_promedio
eficiencia = speedup / numero_de_hilos
```

Variables de entorno opcionales: `VACAS_LIST`, `HILOS_LIST`, `FRAMES`,
`REPETICIONES`, `VACAS_CHUNK`, `VACAS_SCHED`, `ESTRELLAS_LIST` y `SEMILLA`
(ver los comentarios al inicio del script). `REPETICIONES` no acepta menos
de 10, para evitar generar por accidente una bitacora que incumpla la rubrica.

### Verificar la paralelizacion

`--dump-estado` imprime la posicion/velocidad final de cada vaca. Con la
misma semilla, una corrida serial y una paralela deben coincidir byte a
byte:

```bash
./build/zipzip --bench --modo serial --vacas 3000 --frames 30 --semilla 1234 --dump-estado > serial.txt
./build/zipzip --bench --modo paralelo --vacas 3000 --frames 30 --semilla 1234 --hilos 8 --dump-estado > paralelo.txt
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
  Se ejecuta completamente en el hilo principal.
- **Recursos** (`src/assets/`): carga de modelos OBJ.
- **Nucleo compartido** (`include/zipzip/core/`): RNG determinista,
  parametros de camara y cronometro, usados tanto por la simulacion como
  por el modo `--bench`.

## Sincronizacion y memoria compartida

La pasada A de vacas solo lee `Escena::vacas` y cada iteracion escribe en
`ax[i]` y `ay[i]`. La reduccion `min:distanciaMinimaOvni` combina un minimo
privado por hilo sin condiciones de carrera. La barrera implicita al final del
primer `parallel for` garantiza que toda aceleracion este lista antes de la
pasada B. En la pasada B cada hilo modifica una vaca distinta. No se utilizan
`critical` ni mutex porque no hay escrituras concurrentes sobre el mismo dato.

SDL2, OpenGL y todas las funciones de `Renderer` permanecen en el hilo
principal. Las estrellas son seriales por defecto y solo participan en OpenMP
cuando se usa `--estrellas-paralelas` para realizar el experimento dedicado.

## Estado frente a la rubrica

| Requisito | Estado |
|---|---|
| Programa secuencial funcional | Implementado con `--modo serial` |
| Programa OpenMP funcional | Implementado con `--modo paralelo` |
| Comparacion sin duplicar proyecto | Implementada en el mismo ejecutable |
| Programacion defensiva y argumentos | Implementada; usar `--ayuda` |
| N, colores, 640x480+, movimiento, fisica y FPS | Implementado |
| 10 mediciones, speedup y eficiencia | Automatizado con `scripts/` |
| Sincronizacion y proteccion de memoria | Reduccion + barreras implicitas + escrituras disjuntas |
| Historial Git de al menos dos semanas | Cumplido por el historial del repositorio |
| Informe UVG con 3 fuentes | Pendiente del equipo |
| Anexo 1: diagrama de flujo final | Implementado en `docs/diagrama_flujo_final.md`; falta incorporarlo al informe |
| Anexo 2: catalogo de funciones | Implementado en `docs/anexo_02_catalogo_funciones.md` |
| Anexo 3: resultados y capturas reales | Pendiente ejecutar el benchmark completo en el equipo de entrega |
