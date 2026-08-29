# ZipZip  - Proyecto 1 de Computacion Paralela

ZipZip es un screensaver interactivo en vista cenital desarrollado en C++.
La escena representa una granja suspendida en el espacio con varias vacas en
movimiento. Cuando un día llegó un OVNI pilotado por un gato espacial, listo para capturar a las vacas.
Se ven las estrellas, planetas y particulas para representar la captura de las vacas.

El proyecto comienza con una version secuencial que servira como referencia
funcional y de rendimiento. Despues, las actualizaciones independientes de los
elementos se paralelizaran con OpenMP para comparar tiempos, FPS, speedup y
eficiencia con diferentes cantidades de elementos e hilos.

## Integrantes

- Vianka Castro
- Ricardo Godinez
- Abby Donis

## Estado actual

Avances 1 :

- Ventana grafica de 900 x 700 creada con SDL2.
- Renderizado 3D con OpenGL.
- Carga del modelo Wavefront OBJ de una vaca.
- Cantidad configurable de vacas; el valor predeterminado es 20.
- Estado de las vacas almacenado en `std::vector<Instancia>`.
- Posicion, velocidad, giro, escala y color por vaca.
- Movimiento con velocidad pseudoaleatoria y rebote contra los limites.
- HUD con FPS y cantidad de vacas.
- Pausa, wireframe, face culling y control de VSync.


## Estructura actual del repositorio

```text
Proyecto1_Paralela_Zip/
|-- main.cpp              Ventana, eventos, ciclo principal, HUD y renderizado
|-- escena.h              Tipos Instancia y Escena; interfaz de la simulacion
|-- escena.cpp            Creacion, movimiento, giro y rebotes de las vacas
|-- obj_loader.h          Modelo en memoria e interfaz del cargador OBJ
|-- obj_loader.cpp        Lectura, triangulacion y normalizacion de archivos OBJ
|-- cow.obj               Modelo 3D utilizado para representar una vaca
|-- Propuesta Proyecto.pdf
|-- README.md
`-- .gitignore
```


## Compilar y ejecutar en Windows con MSYS2

La opcion recomendada en Windows es la terminal **MSYS2 UCRT64**.

### 1. Abrir la terminal MYS2 UCRT64


```text
C:\msys64\ucrt64.exe
```

### 2. Instalar el compilador y SDL2

Dentro de la terminal MSYS2 UCRT64, ejecutar:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2
```

Cuando pregunte si desea continuar, escribir `Y` y presionar Enter.

### 3. Entrar al repositorio

```bash
cd /c/Proyecto1_Paralela_Zip
```

### 4. Compilar

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  main.cpp obj_loader.cpp escena.cpp \
  -o visor.exe \
  -lmingw32 -lSDL2main -lSDL2 -lopengl32
```

### 5. Ejecutar

```bash
./visor.exe 20 cow.obj
```

El primer argumento es la cantidad de vacas. Por ejemplo:



## Controles

| Tecla | Accion |
|---|---|
| `Espacio` | Pausar o continuar la simulacion |
| `W` | Alternar entre relleno y wireframe |
| `C` | Activar o desactivar face culling |
| `V` | Activar o desactivar VSync |
| `Esc` | Cerrar el programa |

Desactivar VSync permite observar los FPS sin limitarlos a la frecuencia del
monitor.

## Arquitectura prevista

Conforme crezca el proyecto, el codigo se separara en las siguientes areas:

- **Aplicacion:** ventana, eventos y ciclo principal.
- **Simulacion:** vacas, OVNI, estrellas, particulas y reglas de movimiento.
- **Renderizado:** dibujo con OpenGL y HUD.
- **Recursos:** carga de modelos y otros archivos.
- **Rendimiento:** version serial, version OpenMP y mediciones.

Las funciones de actualizacion se disenaran para que cada elemento pueda
procesarse de forma independiente. Las llamadas de SDL2 y OpenGL seguiran en el
hilo principal.
