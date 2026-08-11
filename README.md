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

### Avance 1 completado

| Requisito | Implementacion | Estado |
|---|---|---|
| Ventana grafica y API elegida | `SDL_CreateWindow` y contexto OpenGL en `main.cpp` | Completo |
| Variable en memoria | `Escena::vacas`, un `std::vector<Instancia>` definido en `escena.h` | Completo |
| Renderizado de un elemento | `dibujar` carga y renderiza instancias de `cow.obj` con OpenGL | Completo |
| Actualizacion de ubicacion | `actualizarEscena` modifica `x` y `y` mediante `vx`, `vy` y `dt`, con rebote en los limites | Completo |

El primer commit que contiene todos estos requisitos es
[`b274724d661c268f01373b89c1058f26751ccef5`](https://github.com/Vann06/Proyecto1_Paralela_Zip/commit/b274724d661c268f01373b89c1058f26751ccef5).


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

La simulacion de `escena.cpp` no depende de SDL2 ni de OpenGL. Esta separacion
permite paralelizar posteriormente la actualizacion de las vacas sin ejecutar
llamadas graficas desde los hilos de OpenMP. El renderizado debe permanecer en
el hilo principal.

## Compilar y ejecutar en Windows con MSYS2

En Windows se recomienda utilizar la terminal **MSYS2 UCRT64**. No es necesario
instalar Ubuntu ni WSL.

### 1. Abrir la terminal correcta

1. Presionar la tecla de Windows.
2. Escribir `MSYS2 UCRT64`.
3. Abrir la aplicacion que tenga ese nombre.

En una instalacion predeterminada de MSYS2, tambien se puede abrir:

```text
C:\msys64\ucrt64.exe
```

Si MSYS2 se instalo en otra ubicacion, la ruta anterior sera diferente. El
titulo o prompt de la terminal debe indicar `UCRT64`. No se debe usar `MSYS2
MSYS` para compilar este proyecto.

### 2. Instalar el compilador y SDL2

Dentro de la terminal MSYS2 UCRT64, ejecutar:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2
```

Cuando pregunte si desea continuar, escribir `Y` y presionar Enter.

### 3. Entrar al repositorio

Si todavia no se ha descargado el proyecto:

```bash
git clone https://github.com/Vann06/Proyecto1_Paralela_Zip.git
cd Proyecto1_Paralela_Zip
```

Si el repositorio ya existe, hay que entrar a la carpeta donde cada persona lo
haya guardado. Por ejemplo:

```bash
cd /c/ruta/donde/esta/Proyecto1_Paralela_Zip
```

La ruta `/c/` representa la unidad `C:` de Windows. Se puede comprobar la
ubicacion con:

```bash
pwd
ls
```

El comando `ls` debe mostrar `main.cpp`, `escena.cpp`, `obj_loader.cpp` y
`cow.obj`.

### 4. Compilar

La forma mas sencilla es copiar esta instruccion completa en una sola linea:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp obj_loader.cpp escena.cpp -o visor.exe -lmingw32 -lSDL2main -lSDL2 -lopengl32
```

Si el comando termina sin mensajes de error, se habra creado `visor.exe`.

En Bash, una barra inversa `\` solo continua un comando cuando es el ultimo
caracter de la linea. No se debe escribir `\ main.cpp` ni separar una opcion de
su nombre, como `- lopengl32`.

### 5. Ejecutar

```bash
./visor.exe 20 cow.obj
```

El primer argumento es la cantidad de vacas. Por ejemplo:

```bash
./visor.exe 100 cow.obj
```

Hay que ejecutar el programa desde la raiz del repositorio para que pueda
encontrar `cow.obj`.

## Compilar y ejecutar en Ubuntu

Ubuntu es una alternativa valida si se trabaja directamente en Linux. En WSL se
necesita soporte grafico WSLg para poder mostrar la ventana.

### 1. Instalar dependencias

```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libgl1-mesa-dev
```

### 2. Entrar al repositorio y compilar

```bash
cd /ruta/al/Proyecto1_Paralela_Zip
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp obj_loader.cpp escena.cpp -o visor -lSDL2 -lGL
```

### 3. Ejecutar

```bash
./visor 20 cow.obj
```

## Argumentos

Los argumentos se pueden proporcionar en cualquier orden:

```bash
# Windows
./visor.exe 50 cow.obj
./visor.exe cow.obj 50

# Linux
./visor 50 cow.obj
./visor cow.obj 50
```

- Un argumento numerico indica la cantidad de vacas.
- Un argumento no numerico indica la ruta del modelo OBJ.
- Sin argumentos se crean 20 vacas y se carga `cow.obj`.

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

## Proximos pasos

- Agregar un sistema de compilacion con CMake.
- Separar la aplicacion y el renderizado de `main.cpp`.
- Incorporar el OVNI, estrellas, planetas y particulas.
- Mantener una version serial verificable.
- Paralelizar las actualizaciones independientes con OpenMP.
- Registrar FPS, tiempos, speedup y eficiencia.
- Agregar pruebas para la simulacion y el cargador OBJ.
