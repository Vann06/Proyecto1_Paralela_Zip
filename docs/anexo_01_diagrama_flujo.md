# Anexo 1 - Diagrama de flujo final

Este diagrama documenta el recorrido común de la versión serial y la versión
OpenMP. Ambas usan el mismo código y las mismas estructuras; `--modo` decide
si los lazos de simulación habilitan OpenMP.

```mermaid
flowchart TD
    A([Inicio]) --> B[Leer argumentos]
    B --> C{¿Argumentos y rangos válidos?}
    C -- No --> D[Mostrar error y ayuda\nterminar con código 2]
    C -- Sí --> E{¿Se pidió ayuda?}
    E -- Sí --> F[Mostrar opciones y terminar]
    E -- No --> G{¿Modo paralelo disponible\nen esta compilación?}
    G -- No --> D
    G -- Sí --> H[Configurar modo, hilos,\nschedule y chunk]
    H --> I[Reservar vectores de vacas,\naceleraciones y estrellas]
    I --> J[Crear escena determinista\ncon la semilla indicada]
    J --> K{¿Modo benchmark?}

    K -- Sí --> L[30 frames de calentamiento\nsin SDL ni OpenGL]
    L --> M[Repetir los frames medidos]
    K -- No --> N[Inicializar SDL, OpenGL,\nmodelos y Renderer]
    N --> O[Procesar eventos en hilo principal]
    O --> P{¿Cerrar o Escape?}
    P -- Sí --> Q[Liberar recursos y terminar]
    P -- No --> R[Calcular dt y comenzar cronómetro]

    M --> S[Actualizar OVNI y planetas\nen forma serial]
    R --> S
    S --> T{¿--modo paralelo?}
    T -- No --> U[Pasada A serial:\ninteracción O de N cuadrado]
    T -- Sí --> V[Pasada A omp parallel for:\nescrituras ax i y ay i\nreducción del mínimo]
    V --> W[Barrera implícita OpenMP]
    U --> X[Pasada B serial:\nintegrar y rebotar vacas]
    W --> Y[Pasada B omp parallel for:\nuna vaca distinta por iteración]
    X --> Z{¿Estrellas paralelas?}
    Y --> Z
    Z -- No --> AA[Actualizar estrellas serial]
    Z -- Sí --> AB[Actualizar estrellas con\nomp parallel for]
    AA --> AC[Detener cronómetro de simulación]
    AB --> AC

    AC --> AD{¿Benchmark?}
    AD -- No --> AE[Dibujar todo con Renderer\nen el hilo principal]
    AE --> AF[Intercambiar buffers y\nactualizar FPS/HUD]
    AF --> O
    AD -- Sí --> AG{¿Quedan frames?}
    AG -- Sí --> M
    AG -- No --> AH[Emitir fila CSV con modo, N,\nhilos, tiempos y FPS]
    AH --> AI[benchmark.sh repite cada\nconfiguración al menos 10 veces]
    AI --> AJ[resumir_benchmark.py calcula\npromedio, desviación, speedup y eficiencia]
    AJ --> AK([Resultados para el informe])
```

## Puntos de sincronización y seguridad

- En la pasada A, cada iteración solo escribe `ax[i]` y `ay[i]`; las vacas se
  consultan en modo lectura.
- `reduction(min:distanciaMinimaOvni)` evita una condición de carrera al
  calcular el mínimo global.
- La barrera implícita al final del primer `parallel for` asegura que todas
  las aceleraciones existan antes de integrar posiciones en la pasada B.
- En la pasada B cada iteración modifica una vaca distinta. No se necesita
  `critical`, mutex ni bloqueo explícito.
- SDL, OpenGL, carga de recursos y `Renderer` permanecen en el hilo principal.

