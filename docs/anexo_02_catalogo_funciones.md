# Anexo 2 - Catálogo de funciones

El catálogo enumera las funciones relevantes del programa, sus entradas,
salidas y responsabilidad. Las funciones internas pequeñas se agrupan cuando
pertenecen a una sola tarea de implementación.

## Entrada, configuración y medición

| Función | Entradas | Salida | Descripción |
|---|---|---|---|
| `leerEnteroEnRango` | texto, mínimo, máximo, referencia de destino | `bool` | Convierte un entero sin excepciones y rechaza texto parcial o valores fuera de rango. |
| `leerSemilla` | texto y referencia de destino | `bool` | Valida una semilla positiva de 32 bits. |
| `imprimirAyuda` | nombre del ejecutable | ninguna | Muestra todas las opciones disponibles. |
| `leerArgumentos` | `argc`, `argv`, opciones y mensaje de error | `bool` | Interpreta argumentos nombrados y la sintaxis posicional compatible. |
| `procesarEventos` | cola de eventos SDL | `bool` | Mantiene la ejecución o solicita cierre con Escape/la X. Solo hilo principal. |
| `configurarOpenMP` | opciones de modo, hilos y schedule | hilos activos | Configura el runtime OpenMP; el modo serial fuerza un hilo y desactiva los pragmas mediante su condición. |
| `usarOpenMP` | opciones | `bool` | Indica si el ejecutable tiene OpenMP y el usuario eligió modo paralelo. |
| `nombreModo` | opciones | texto | Devuelve `serial` o `paralelo` para HUD y CSV. |
| `imprimirEstadoDump` | escena final | ninguna | Imprime posiciones y velocidades para comparar resultados deterministas. |
| `ejecutarBench` | opciones | código de salida | Ejecuta calentamiento y medición sin ventana; emite una fila CSV. |
| `main` | argumentos del proceso | código de salida | Coordina validación, benchmark o ciclo gráfico y libera recursos. |
| `Cronometro::iniciar/detener` | bloque medido | ninguna | Registra la duración de una muestra con `steady_clock`. |
| `Cronometro::registrarMuestra` | milisegundos | ninguna | Agrega una medición ya calculada. |
| `Cronometro::limpiar` | ninguna | ninguna | Elimina las muestras acumuladas. |
| `Cronometro::cantidadMuestras` | ninguna | cantidad | Devuelve el número de muestras. |
| `Cronometro::promedioMs/minimoMs/maximoMs/p95Ms` | muestras internas | milisegundos | Resume el rendimiento observado. |

## Núcleo determinista y cámara

| Función | Entradas | Salida | Descripción |
|---|---|---|---|
| `zipzip::xorshift32` | estado mutable | entero pseudoaleatorio | Avanza el generador determinista. |
| `zipzip::uniformeEntre` | estado, límite inferior y superior | `float` | Produce un valor uniforme en el intervalo solicitado. |
| `Rng::siguiente/entre` | estado propio y, en `entre`, límites | entero o `float` | Interfaz con estado para generar escenas reproducibles. |
| `zipzip::mitadAltoVisible` | constantes de cámara | `float` | Calcula trigonométricamente la mitad de la altura visible en `z = 0`. |

## Carga de modelos OBJ

| Función | Entradas | Salida | Descripción |
|---|---|---|---|
| `cargarOBJ` | ruta, modelo destino, opciones y error opcional | `bool` | Lee vértices/caras y grupos, triangula, genera normales faltantes y normaliza la escala. |
| `parseCorner` | token de una cara OBJ | índices por referencia | Separa índices de posición, textura y normal. |
| `resolver` | índice OBJ y cantidad disponible | índice normalizado | Convierte índices positivos o negativos de OBJ. |
| `normalizarEscala` | posiciones y tamaño objetivo | ninguna | Centra y escala la geometría. |
| `calcularNormalesSuaves` | posiciones y triángulos | normales | Calcula normales cuando el archivo no las contiene. |
| `resta/cruz` | vectores 3D | vector 3D | Operaciones auxiliares para normales. |
| `fallar` | destino opcional y mensaje | `false` | Centraliza los errores defensivos del cargador. |

## Simulación

| Función | Entradas | Salida | Descripción y paralelismo |
|---|---|---|---|
| `desdeTono` | tono y tres canales por referencia | ninguna | Convierte un tono en color RGB para las vacas. |
| `crearEscena` | escena, N, dimensiones y semilla | ninguna | Reserva memoria e inicializa vacas, aceleraciones, planetas y OVNI. |
| `actualizarEscena` | escena, `dt`, bandera OpenMP y tiempos opcionales | ninguna | Actualiza OVNI/planetas; ejecuta la interacción O(N²) y la integración O(N), seriales o OpenMP sobre los mismos datos. |
| `prepararFugaz` | estrella fugaz, campo y espera | ninguna | Inicializa de forma determinista una nueva trayectoria fugaz. |
| `crearCampoEstrellas` | campo, cantidad, dimensiones y semilla | ninguna | Reserva e inicializa estrellas normales y fugaces. |
| `actualizarCampoEstrellas` | campo, `dt` y bandera OpenMP | ninguna | Actualiza movimiento y brillo; el lazo de estrellas solo usa OpenMP cuando se pide explícitamente. |

## Renderizado (siempre en el hilo principal)

| Función | Entradas | Salida | Descripción |
|---|---|---|---|
| `Renderer::Renderer` | ancho y alto | objeto | Guarda las dimensiones del canvas. |
| `Renderer::inicializar` | contexto OpenGL activo | ninguna | Configura OpenGL y prepara recursos gráficos. |
| `Renderer::liberarRecursos` | recursos internos | ninguna | Elimina texturas y demás recursos creados. |
| `Renderer::configurarProyeccion` | dimensiones internas | ninguna | Configura perspectiva y viewport. |
| `cargarTexturaBMP` | ruta, identificador y modo de repetición | `bool` | Carga un BMP con SDL2, lo convierte a RGBA y lo sube a OpenGL; permite continuar si falla. |
| `Renderer::prepararTexturasEscena` | rutas convencionales de `assets/textures` | ninguna | Prepara el fondo espacial y la textura repetible de grama. |
| `Renderer::mitadAltoVisible` | cámara | `float` | Expone la dimensión visible usada también por simulación. |
| `Renderer::prepararEsferaPlaneta` | ninguna | ninguna | Genera una malla esférica reutilizable. |
| `Renderer::prepararTexturasPlanetas` | estilos internos | ninguna | Genera y sube las cuatro texturas procedurales. |
| `Renderer::alternarWireframe/alternarCulling` | estado interno | ninguna | Cambia opciones de depuración gráfica. |
| `Renderer::dibujar` | modelos, escena, estrellas y métricas | ninguna | Dibuja un frame completo y llama a las capas especializadas. |
| `Renderer::dibujarFondo` | textura de fondo y dimensiones | ninguna | Dibuja la imagen espacial como un quad ortográfico detrás de la escena. |
| `Renderer::dibujarEstrellas` | campo | ninguna | Dibuja fondo, parpadeo y estrellas fugaces. |
| `Renderer::dibujarPlataforma` | escena | ninguna | Dibuja la cúpula semicircular. |
| `Renderer::dibujarVacas` | modelo y escena | ninguna | Renderiza cada instancia con su transformación y color. |
| `Renderer::dibujarOvni` | modelo y escena | ninguna | Dibuja el OVNI en su posición simulada. |
| `Renderer::dibujarPlanetas` | escena | ninguna | Dibuja planetas, texturas, rotación y aros. |
| `Renderer::dibujarHUD` | FPS, cantidades, tiempo, hilos y mínimo | ninguna | Muestra las métricas académicas sobre la escena. |
| `buscarGlifo/dibujarTexto` | carácter o texto y posición | glifo o ninguna | Implementan la fuente del HUD. |
| `convertirCanal/transicionSuave/generarTexturaPlaneta` | valores o estilo | canal, interpolación o píxeles | Construyen las texturas procedurales en el hilo principal. |

## Automatización de resultados

| Componente | Entradas | Salida | Descripción |
|---|---|---|---|
| `scripts/benchmark.sh` | ejecutable y variables de experimento | CSV crudo | Ejecuta serial/paralelo, schedules, chunks y estrellas con al menos 10 repeticiones por configuración. |
| `scripts/resumir_benchmark.py` | CSV crudo | CSV resumido | Agrupa mediciones y calcula promedio, desviación estándar, speedup y eficiencia. |
