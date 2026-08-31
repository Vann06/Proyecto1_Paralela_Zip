# Cómo funciona ZipZip — El mapa de la paralelización (OpenMP)

Este documento asume que ya leíste
[`flujo_02_simulacion.md`](flujo_02_simulacion.md), donde se explicó la idea
general de "repartir el trabajo entre varios hilos" con la analogía de
**contratar ayudantes** en vez de hacer todo tú solo.

Este documento es distinto: es un **mapa exacto**. Para cada uno de los
lugares del código donde de verdad se "contrató ayuda extra", vas a
encontrar aquí:

- **En qué archivo y en qué línea está.**
- **La línea de código exacta.**
- **Qué tarea se reparte**, explicada con la analogía de contratar personas.
- **Si valió la pena** contratar ayuda ahí, o si el trabajo era tan chico
  que casi no importa.

La idea es que abras el archivo, busques la línea, y digas "ahh, por *esto*
metieron más gente a trabajar en vez de una sola persona".

## La frase mágica que hay que buscar

En C++, la instrucción que le dice al programa "reparte lo que sigue entre
varios ayudantes" es una línea que empieza así:

```cpp
#pragma omp parallel for
```

Busca esa frase (o `pragma omp` a secas) en los archivos del proyecto. Hay
**dos lazos principales de vacas** y un tercero opcional para estrellas. El
resto, especialmente SDL/OpenGL y `Renderer`, permanece en el hilo principal.

## Primero: ¿quién decide *cuántos* ayudantes se contratan?

Antes de ver los 4 lugares donde se reparte trabajo, hay que entender quién
decide **cuánta gente hay disponible para ayudar**. Eso pasa en un solo
lugar, `src/main.cpp`, en una función llamada `configurarOpenMP`:

```cpp
// src/main.cpp, función configurarOpenMP
int configurarOpenMP(const Opciones& opciones) {
    if (opciones.modo == ModoEjecucion::Serial) return 1;
    if (opciones.hilos > 0) omp_set_num_threads(opciones.hilos);
    // ...decide si repartir el trabajo en bloques iguales de antemano
    // ('static'), o de a poquitos conforme cada quien va terminando
    // ('dynamic' / 'guided')...
    omp_set_schedule(tipo, opciones.chunk > 0 ? opciones.chunk : 0);

    return omp_get_max_threads();
}
```

En la analogía: esto es el momento de **"voy a contratar exactamente 8
personas para hoy"** (`omp_set_num_threads`) y de **decidir cómo les voy a
repartir la lista de tareas** (`omp_set_schedule`):

- `static` — le doy a cada persona un bloque fijo de tareas *desde el
  principio* ("tú te encargas de las vacas 1 a 375, tú de la 376 a 750...").
  Rápido de organizar, pero si algunas tareas resultan más difíciles que
  otras, alguien puede terminar mucho antes que los demás y quedarse sin
  hacer nada mientras espera.
- `dynamic` — reparto las tareas de a poquitos, y en cuanto alguien termina
  su pedacito, le doy el siguiente. Más parejo cuando las tareas no cuestan
  todas lo mismo, pero organizar cada entrega tiene su propio costillo.

Esto se controla desde afuera del programa, sin tocar el código, con las
opciones `--modo`, `--hilos` y `--schedule`:

```bash
./zipzip --modo paralelo --vacas 3000 --hilos 8 --schedule dynamic
```

## Los lugares donde se reparte trabajo

### 1. Las vacas se separan entre sí — el más importante

**Dónde:** `src/simulation/scene.cpp`, línea 216, dentro de la función
`actualizarEscena` (la "Pasada A" del documento 2).

```cpp
#pragma omp parallel for if(usarOpenMP) schedule(runtime) \
    reduction(min:distanciaMinimaOvni)
for (long indiceVaca = 0; indiceVaca < cantidadVacas; ++indiceVaca) {
    // ...revisar a TODAS las demás vacas y decidir si hay que
    // alejarse de alguna que esté muy cerca...
}
```

**La tarea que se reparte:** "para cada una de las N vacas, revisa a *todas*
las demás y decide hacia dónde debería empujarse". Si hay 3,000 vacas, eso
son hasta 3,000 × 3,000 ≈ 9 millones de comparaciones por cada frame.

**La analogía:** imagina una lista de 3,000 personas donde cada una tiene que
comparar su posición con las otras 2,999. Si le das la lista completa a una
sola persona, tarda muchísimo. Si contratas 8 personas y le das a cada una
375 nombres de la lista para que revisen (cada quien sigue comparando contra
las 3,000, pero solo le toca *iniciar* la revisión de 375), el trabajo total
se reparte casi 8 veces más rápido.

**¿Valió la pena?** **Sí, mucho.** Esta es la tarea más pesada de todo el
programa — cada vaca hace bastante trabajo real (raíces cuadradas, divisiones)
por cada vecina que revisa. Es la razón principal por la que el proyecto
puede mostrar una diferencia clara entre correr con 1 hilo y correr con 16.

### 2. Cada vaca se mueve según lo que decidió — el "más barato"

**Dónde:** `src/simulation/scene.cpp`, línea 278, un poco más abajo en la
misma función (la "Pasada B" del documento 2).

```cpp
#pragma omp parallel for if(usarOpenMP) schedule(runtime)
for (long indiceVaca = 0; indiceVaca < cantidadVacas; ++indiceVaca) {
    Instancia& vaca = e.vacas[static_cast<size_t>(indiceVaca)];
    // ...mover a ESTA vaca según lo que decidió en el paso anterior,
    // y rebotar si se salió de la plataforma...
}
```

**La tarea que se reparte:** "para cada vaca, actualiza su propia posición".
A diferencia de la anterior, aquí cada vaca **solo mira su propio dato** —
no revisa a las demás.

**La analogía:** ahora cada una de las 3,000 personas tiene una tarjetita
con instrucciones cortas ("camina 2 pasos al norte") y solo tiene que
seguirlas — no tiene que consultar con nadie más. Repartir esto entre 8
personas sigue ayudando, pero cada tarjetita es tan rápida de leer que gran
parte del tiempo se va en **organizar** quién hace cuál, no en el trabajo en
sí.

**¿Valió la pena?** **Un poco, pero mucho menos que la anterior.** Esta
tarea es "barata" por vaca (pocas cuentas), así que el beneficio de
repartirla entre más gente se aplana rápido. Este contraste — la tarea 1 sí
escala bien, la tarea 2 casi no — es justo el resultado que el proyecto
quiere mostrar en el informe.

### 3. Las estrellas de fondo: experimento opcional

**Dónde:** `src/simulation/starfield.cpp`, en
`actualizarCampoEstrellas`.

```cpp
#pragma omp parallel for if(usarOpenMP) schedule(runtime)
for (long i = 0; i < cantidadEstrellas; ++i) {
    Estrella& estrella = campo.estrellas[static_cast<size_t>(i)];
    // ...moverla un poquito, y si se salió de la pantalla, reaparecerla
    // del otro lado...
}
```

**La tarea que se reparte:** mover cada una de las ~180 estrellas de fondo.
Igual que la vaca en la Pasada B, cada estrella solo depende de sí misma.

**La analogía:** 180 personas, cada una con una tarjetita cortísima ("muévete
un pasito"). Aquí contratar ayudantes casi no se nota, porque hay pocas
estrellas y cada una es baratísima de mover — el tiempo que se ahorra
repartiendo el trabajo es menor que el tiempo que cuesta organizar quién
hace qué.

**¿Valió la pena?** Con 180 estrellas, normalmente **no**: el costo de formar
el equipo OpenMP supera el trabajo ahorrado. Por eso el lazo recibe `false`
por defecto y solo se activa con `--estrellas-paralelas`. El experimento 4 de
`scripts/benchmark.sh` repite la comparación con 180 y 10,000 estrellas para
determinar a partir de qué M comienza a convenir.

Las texturas procedurales de planetas ya no se paralelizan. Aunque cada pixel
es independiente, esa preparación pertenece al módulo gráfico y ocurre solo
al inicio. Mantenerla serial hace inequívoca la regla académica del proyecto:
la simulación puede usar OpenMP; SDL, OpenGL y `Renderer` no.

## Resumen en una tabla

| # | Archivo y línea | Qué se reparte | ¿Cuánto ayuda? |
|---|---|---|---|
| 1 | `scene.cpp`, pasada A | Cada vaca revisa a todas las demás (separación) | **Mucho** — la tarea principal del proyecto |
| 2 | `scene.cpp`, pasada B | Cada vaca mueve solo su propia posición | Poco — tarea barata por vaca |
| 3 | `starfield.cpp` | Cada estrella se mueve sola | Opcional; con 180 estrellas suele perjudicar |

## Sincronización entre las dos pasadas

OpenMP coloca una **barrera implícita** al final de la Pasada A. Ningún hilo
puede comenzar la Pasada B hasta que todos hayan terminado de escribir
`ax[i]` y `ay[i]`. En la Pasada A cada hilo escribe índices distintos y la
distancia mínima usa `reduction(min:...)`, que combina mínimos privados de
forma segura. En la Pasada B cada hilo modifica una vaca distinta. No hace
falta un mutex ni una sección `critical` porque no existen dos hilos
escribiendo el mismo elemento.

## Y todo lo demás, ¿por qué no se repartió?

Esto es igual de importante para entender el panorama completo. El resto del
programa sigue haciéndolo una sola persona (un solo hilo), a propósito:

- **Mover el OVNI y los 4 planetas** (`scene.cpp`): son tan pocos elementos
  (1 OVNI, 4 planetas) que ni vale la pena organizar ayudantes — sería como
  contratar 8 personas para cargar 4 cajas: la mayoría se queda sin hacer
  nada, y organizar quién carga cuál cuesta más que cargarlas tú mismo.
- **Dibujar en pantalla y crear recursos** (todo `renderer.cpp`): dibujar
  usa una herramienta (OpenGL) que solo puede recibir instrucciones **una a
  la vez**, en un orden específico — es como un pintor con un solo pincel:
  no puedes repartir "pintar el cuadro" entre 8 personas si solo hay un
  pincel y un lienzo. Por eso, aunque dibujar cada vaca es una tarea que se
  repite muchas veces, no se reparte entre hilos.
- **Cargar los archivos `.obj`** (`obj_loader.cpp`): esto pasa **una sola
  vez**, al arrancar el programa, no en cada frame — organizar ayudantes
  para algo que ocurre una sola vez casi nunca vale la pena.
- **El ciclo principal en sí** (`main.cpp`): decidir "¿sigo corriendo o ya
  me cierro?", "¿cuánto tiempo pasó?" son pasos que dependen unos de otros
  en orden estricto — no se pueden repartir entre varias personas porque
  cada paso necesita el resultado del anterior.

## Cómo comprobarlo con tus propios ojos

Si quieres *ver* esto pasando, no solo leerlo:

1. Abre el Monitor de sistema / Administrador de tareas de tu computadora
   (o `htop` en Linux) **antes** de correr el programa, para ver cuántos
   núcleos tiene tu procesador.
2. Corre el programa explícitamente en serial:
   `./zipzip --modo serial --vacas 5000`. Vas a ver que solo un núcleo del
   procesador se pone al 100% mientras los demás casi no se mueven.
3. Cierra y corre lo mismo en paralelo:
   `./zipzip --modo paralelo --vacas 5000 --hilos 8`. Ahora deberías ver varios núcleos
   trabajando al mismo tiempo — literalmente varias "personas" ayudando a
   la vez — y el número de FPS en el título de la ventana debería subir.

Eso es, en la práctica, la contratación de ayudantes ocurriendo frente a
tus ojos.
