#include "zipzip/assets/obj_loader.h"
#include "zipzip/core/camara.h"
#include "zipzip/core/cronometro.h"
#include "zipzip/rendering/renderer.h"
#include "zipzip/simulation/scene.h"
#include "zipzip/simulation/starfield.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

constexpr int ANCHO = 900;
constexpr int ALTO = 700;
constexpr int VACAS_POR_DEFECTO = 20;
constexpr int ESTRELLAS_POR_DEFECTO = 180;
constexpr int FRAMES_BENCH_POR_DEFECTO = 300;

enum class ModoEjecucion {
    Serial,
    Paralelo
};

struct Opciones {
    int ancho = ANCHO;
    int alto = ALTO;
    int cantidadVacas = VACAS_POR_DEFECTO;
    int cantidadEstrellas = ESTRELLAS_POR_DEFECTO;
    std::string rutaModelo = "assets/models/cow.obj";
    std::uint32_t semilla = 1234u;
#ifdef _OPENMP
    ModoEjecucion modo = ModoEjecucion::Paralelo;
#else
    ModoEjecucion modo = ModoEjecucion::Serial;
#endif
    bool paralelizarEstrellas = false;

    // Cuantos hilos usa OpenMP. 0 significa "el valor por defecto del
    // entorno" (OMP_NUM_THREADS o los nucleos disponibles).
    int hilos = 0;
    std::string schedule = "static";  // static | dynamic | guided

    // Tamano de bloque del schedule. 0 deja que OpenMP use su default (para
    // 'static', bloques ~N/hilos). Fijarlo en 1 con --schedule static es lo
    // que expone false sharing entre vacas consecutivas: cada hilo termina
    // escribiendo en Instancia intercaladas que caen en la misma linea de
    // cache (Instancia mide 40 bytes, la linea son 64).
    int chunk = 0;

    // Modo sin ventana: corre la simulacion 'frames' veces con dt fijo y
    // escribe una fila CSV a stdout. Sirve para generar la tabla de
    // speedup/eficiencia sin que el render sea el cuello de botella.
    bool bench = false;
    int frames = FRAMES_BENCH_POR_DEFECTO;

    // Imprime el estado final (posiciones) para comparar bit a bit entre
    // una corrida serial y una paralela con la misma semilla.
    bool dumpEstado = false;
    bool mostrarAyuda = false;
};

bool leerEnteroEnRango(const std::string& texto, int minimo, int maximo,
                       int& resultado) {
    int valor = 0;
    const char* inicio = texto.data();
    const char* fin = inicio + texto.size();
    const auto conversion = std::from_chars(inicio, fin, valor);
    if (conversion.ec != std::errc() || conversion.ptr != fin ||
        valor < minimo || valor > maximo) {
        return false;
    }
    resultado = valor;
    return true;
}

bool leerSemilla(const std::string& texto, std::uint32_t& resultado) {
    std::uint32_t valor = 0;
    const char* inicio = texto.data();
    const char* fin = inicio + texto.size();
    const auto conversion = std::from_chars(inicio, fin, valor);
    if (conversion.ec != std::errc() || conversion.ptr != fin || valor == 0u) {
        return false;
    }
    resultado = valor;
    return true;
}

void imprimirAyuda(const char* programa) {
    std::cout
        << "Uso: " << programa << " [N] [opciones]\n"
        << "  --modo serial|paralelo      Modo de la simulacion\n"
        << "  --vacas N                   Cantidad de vacas (1..100000)\n"
        << "  --estrellas N               Cantidad de estrellas (1..1000000)\n"
        << "  --estrellas-paralelas       Evaluar estrellas tambien con OpenMP\n"
        << "  --hilos N                   Hilos OpenMP (1..1024)\n"
        << "  --schedule static|dynamic|guided\n"
        << "  --chunk N                   Bloque del schedule (0 = automatico)\n"
        << "  --ancho N --alto N          Canvas (minimo 640x480)\n"
        << "  --semilla N                 Semilla reproducible (> 0)\n"
        << "  --modelo ruta               Modelo OBJ de las vacas\n"
        << "  --bench --frames N          Medicion sin ventana\n"
        << "  --dump-estado               Estado final para comprobar carreras\n"
        << "  --ayuda                     Mostrar este texto\n";
}

bool leerArgumentos(int argc, char* argv[], Opciones& opciones,
                     std::string& error) {
    bool modeloPosicionalAsignado = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto siguienteValor = [&](const char* nombre, std::string& valor) {
            if (i + 1 >= argc) {
                error = std::string("Falta el valor de ") + nombre;
                return false;
            }
            valor = argv[++i];
            return true;
        };

        auto enteroOError = [&](const char* nombre, int minimo, int maximo,
                                int& destino) {
            std::string texto;
            if (!siguienteValor(nombre, texto)) return false;
            if (!leerEnteroEnRango(texto, minimo, maximo, destino)) {
                error = std::string("Valor invalido para ") + nombre +
                        ": '" + texto + "' (rango " +
                        std::to_string(minimo) + ".." +
                        std::to_string(maximo) + ")";
                return false;
            }
            return true;
        };

        if (arg == "--vacas") {
            if (!enteroOError("--vacas", 1, 100000,
                              opciones.cantidadVacas)) return false;
        } else if (arg == "--estrellas") {
            if (!enteroOError("--estrellas", 1, 1000000,
                              opciones.cantidadEstrellas)) return false;
        } else if (arg == "--hilos") {
            if (!enteroOError("--hilos", 1, 1024, opciones.hilos)) return false;
        } else if (arg == "--schedule") {
            if (!siguienteValor("--schedule", opciones.schedule)) return false;
            if (opciones.schedule != "static" &&
                opciones.schedule != "dynamic" &&
                opciones.schedule != "guided") {
                error = "--schedule debe ser static, dynamic o guided";
                return false;
            }
        } else if (arg == "--chunk") {
            if (!enteroOError("--chunk", 0, 1000000, opciones.chunk)) return false;
        } else if (arg == "--modelo") {
            if (!siguienteValor("--modelo", opciones.rutaModelo)) return false;
            if (opciones.rutaModelo.empty()) {
                error = "--modelo no puede estar vacio";
                return false;
            }
        } else if (arg == "--modo") {
            std::string modo;
            if (!siguienteValor("--modo", modo)) return false;
            if (modo == "serial") opciones.modo = ModoEjecucion::Serial;
            else if (modo == "paralelo") opciones.modo = ModoEjecucion::Paralelo;
            else {
                error = "--modo debe ser serial o paralelo";
                return false;
            }
        } else if (arg == "--estrellas-paralelas") {
            opciones.paralelizarEstrellas = true;
        } else if (arg == "--ancho") {
            if (!enteroOError("--ancho", 640, 7680, opciones.ancho)) return false;
        } else if (arg == "--alto") {
            if (!enteroOError("--alto", 480, 4320, opciones.alto)) return false;
        } else if (arg == "--semilla") {
            std::string texto;
            if (!siguienteValor("--semilla", texto)) return false;
            if (!leerSemilla(texto, opciones.semilla)) {
                error = "Valor invalido para --semilla: '" + texto + "'";
                return false;
            }
        } else if (arg == "--bench") {
            opciones.bench = true;
        } else if (arg == "--frames") {
            if (!enteroOError("--frames", 1, 100000,
                              opciones.frames)) return false;
        } else if (arg == "--dump-estado") {
            opciones.dumpEstado = true;
        } else if (arg == "--ayuda" || arg == "--help" || arg == "-h") {
            opciones.mostrarAyuda = true;
        } else {
            // Compatibilidad con la forma antigua: un numero suelto es la
            // cantidad de vacas; cualquier otra cosa es la ruta del modelo.
            int cantidad = 0;
            if (leerEnteroEnRango(arg, 1, 100000, cantidad)) {
                opciones.cantidadVacas = cantidad;
            } else if (!arg.empty() && arg.front() == '-') {
                error = "Opcion desconocida: " + arg;
                return false;
            } else if (!modeloPosicionalAsignado) {
                opciones.rutaModelo = arg;
                modeloPosicionalAsignado = true;
            } else {
                error = "Argumento posicional inesperado: " + arg;
                return false;
            }
        }
    }
    return true;
}

// Sin controles de simulacion (pausa/wireframe/culling/vsync): la ventana
// solo corre a maxima velocidad y se cierra con Esc o la 'x', que es lo que
// hace falta para comprobar visualmente el efecto de la paralelizacion.
bool procesarEventos() {
    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) return false;
        if (evento.type == SDL_KEYDOWN &&
            evento.key.keysym.sym == SDLK_ESCAPE) {
            return false;
        }
    }
    return true;
}

// Aplica --hilos y --schedule al entorno de OpenMP y devuelve la cantidad de
// hilos que realmente quedo activa (1 si el binario se compilo sin OpenMP).
// El schedule se fija aqui y no en cada pragma para poder compararlo desde
// la linea de comandos sin recompilar: los pragmas usan schedule(runtime).
int configurarOpenMP(const Opciones& opciones) {
#ifdef _OPENMP
    if (opciones.modo == ModoEjecucion::Serial) {
        omp_set_num_threads(1);
        return 1;
    }
    if (opciones.hilos > 0) omp_set_num_threads(opciones.hilos);

    omp_sched_t tipo = omp_sched_static;
    if (opciones.schedule == "dynamic") tipo = omp_sched_dynamic;
    else if (opciones.schedule == "guided") tipo = omp_sched_guided;
    else if (opciones.schedule != "static") {
        std::cerr << "Aviso: schedule desconocido '" << opciones.schedule
                  << "', se usa 'static'\n";
    }
    omp_set_schedule(tipo, opciones.chunk > 0 ? opciones.chunk : 0);

    return omp_get_max_threads();
#else
    (void)opciones;
    return 1;
#endif
}

bool usarOpenMP(const Opciones& opciones) {
#ifdef _OPENMP
    return opciones.modo == ModoEjecucion::Paralelo;
#else
    (void)opciones;
    return false;
#endif
}

const char* nombreModo(const Opciones& opciones) {
    return usarOpenMP(opciones) ? "paralelo" : "serial";
}

// Imprime posicion y velocidad de cada vaca con precision suficiente para
// diferenciar corridas bit a bit. Con la misma semilla, una corrida con
// --hilos 1 y otra con --hilos N deben producir exactamente la misma salida;
// si no coinciden, hay una condicion de carrera en la paralelizacion.
void imprimirEstadoDump(const Escena& escena) {
    for (size_t indiceVaca = 0; indiceVaca < escena.vacas.size(); ++indiceVaca) {
        const Instancia& vaca = escena.vacas[indiceVaca];
        std::printf("%zu %.9f %.9f %.9f %.9f %.9f\n",
                    indiceVaca, vaca.x, vaca.y, vaca.vx, vaca.vy, vaca.giro);
    }
}

// Modo sin ventana: corre la simulacion 'frames' veces con dt fijo (no el
// reloj real, para que la corrida sea reproducible) y mide solo el costo de
// actualizarEscena/actualizarCampoEstrellas. No inicializa SDL ni OpenGL, asi
// que N puede subir mucho mas alla de lo que el render podria dibujar, que es
// necesario para que el trabajo por hilo justifique el overhead de OpenMP.
int ejecutarBench(const Opciones& opciones) {
    constexpr float DT_FIJO = 1.0f / 60.0f;
    constexpr int FRAMES_CALENTAMIENTO = 30;

    const int hilos = configurarOpenMP(opciones);
    const bool paralelo = usarOpenMP(opciones);
    const bool estrellasParalelas = paralelo && opciones.paralelizarEstrellas;

    const float mitadAlto = zipzip::mitadAltoVisible();
    const float mitadAncho = mitadAlto *
        (static_cast<float>(opciones.ancho) / opciones.alto);

    Escena escena;
    crearEscena(escena, opciones.cantidadVacas, mitadAncho, mitadAlto,
                opciones.semilla);

    CampoEstrellas campoEstrellas;
    crearCampoEstrellas(campoEstrellas, opciones.cantidadEstrellas,
                        mitadAncho, mitadAlto,
                        opciones.semilla ^ 0x9E3779B9u);

    for (int indiceFrame = 0; indiceFrame < FRAMES_CALENTAMIENTO; ++indiceFrame) {
        actualizarEscena(escena, DT_FIJO, paralelo);
        actualizarCampoEstrellas(campoEstrellas, DT_FIJO,
                                 estrellasParalelas);
    }

    // Se mide el total y, por separado, cada pasada de actualizarEscena.
    // Experimentos como false sharing (schedule static,1) viven en la
    // pasada B (O(N), memory-bound) y quedan diluidos si solo se mira el
    // total, que la pasada A (O(N^2)) domina en cuanto N crece.
    Cronometro cronometroSim;
    Cronometro cronometroPasadaA;
    Cronometro cronometroPasadaB;
    for (int indiceFrame = 0; indiceFrame < opciones.frames; ++indiceFrame) {
        double msPasadaA = 0.0;
        double msPasadaB = 0.0;
        cronometroSim.iniciar();
        actualizarEscena(escena, DT_FIJO, paralelo,
                         &msPasadaA, &msPasadaB);
        actualizarCampoEstrellas(campoEstrellas, DT_FIJO,
                                 estrellasParalelas);
        cronometroSim.detener();
        cronometroPasadaA.registrarMuestra(msPasadaA);
        cronometroPasadaB.registrarMuestra(msPasadaB);
    }

    if (opciones.dumpEstado) {
        imprimirEstadoDump(escena);
        return 0;
    }

    // Sin render que medir en este modo: la columna existe para que el CSV
    // tenga la misma forma que un futuro modo con ventana, pero queda en 0.
    // 'fps' aqui es 1000/ms_sim, es decir cuadros de simulacion por segundo,
    // no cuadros dibujados.
    const double msSimAvg = cronometroSim.promedioMs();
    const double fpsSim = msSimAvg > 0.0 ? 1000.0 / msSimAvg : 0.0;

    // El encabezado va a stderr (no a stdout) para que varias invocaciones
    // de --bench se puedan concatenar en un solo CSV sin repetirlo; vease
    // scripts/benchmark.sh, que imprime el encabezado una sola vez.
    // ms_pasada_a = interaccion O(N^2) entre vacas (compute-bound).
    // ms_pasada_b = integracion O(N) (memory-bound, aqui vive false sharing).
    std::fprintf(stderr,
        "modo,vacas,estrellas,hilos,estrellas_paralelas,schedule,chunk,"
        "semilla,ms_sim_avg,ms_sim_p95,ms_pasada_a_avg,ms_pasada_b_avg,"
        "ms_render_avg,fps\n");
    std::printf("%s,%d,%d,%d,%d,%s,%d,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.3f\n",
                nombreModo(opciones), opciones.cantidadVacas,
                opciones.cantidadEstrellas, hilos,
                estrellasParalelas ? 1 : 0, opciones.schedule.c_str(),
                opciones.chunk, opciones.semilla, msSimAvg,
                cronometroSim.p95Ms(), cronometroPasadaA.promedioMs(),
                cronometroPasadaB.promedioMs(), 0.0, fpsSim);
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    Opciones opciones;
    std::string errorArgumentos;
    if (!leerArgumentos(argc, argv, opciones, errorArgumentos)) {
        std::cerr << "Error: " << errorArgumentos << "\n\n";
        imprimirAyuda(argv[0]);
        return 2;
    }
    if (opciones.mostrarAyuda) {
        imprimirAyuda(argv[0]);
        return 0;
    }

#ifndef _OPENMP
    if (opciones.modo == ModoEjecucion::Paralelo) {
        std::cerr << "Error: este ejecutable fue compilado sin OpenMP; "
                     "usa --modo serial o recompila con ZIPZIP_OPENMP=ON\n";
        return 2;
    }
#endif
    if (opciones.modo == ModoEjecucion::Serial &&
        opciones.paralelizarEstrellas) {
        std::cerr << "Error: --estrellas-paralelas requiere --modo paralelo\n";
        return 2;
    }

    if (opciones.bench) {
        return ejecutarBench(opciones);
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* ventana = SDL_CreateWindow(
        "ZipZip Espacial",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        opciones.ancho,
        opciones.alto,
        SDL_WINDOW_OPENGL);

    if (!ventana) {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext contexto = SDL_GL_CreateContext(ventana);
    if (!contexto) {
        SDL_Log("SDL_GL_CreateContext: %s", SDL_GetError());
        SDL_DestroyWindow(ventana);
        SDL_Quit();
        return 1;
    }

    // Sin VSync: el frame corre tan rapido como pueda, sin taparse en 60 FPS,
    // para que el efecto de la paralelizacion se note en los FPS mostrados.
    SDL_GL_SetSwapInterval(0);
    SDL_Log("OpenGL: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    SDL_Log("Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    Modelo modelo;
    std::string error;
    if (!cargarOBJ(opciones.rutaModelo, modelo, OpcionesOBJ(), &error)) {
        SDL_Log("Error cargando modelo: %s", error.c_str());
        SDL_GL_DeleteContext(contexto);
        SDL_DestroyWindow(ventana);
        SDL_Quit();
        return 1;
    }

    Modelo modeloOvni;
    if (!cargarOBJ("assets/models/ufo_gato.obj", modeloOvni, OpcionesOBJ(),
                    &error)) {
        SDL_Log("Error cargando modelo del OVNI: %s", error.c_str());
        SDL_GL_DeleteContext(contexto);
        SDL_DestroyWindow(ventana);
        SDL_Quit();
        return 1;
    }

    const int hilos = configurarOpenMP(opciones);

    Renderer renderer(opciones.ancho, opciones.alto);
    renderer.inicializar();

    const float mitadAlto = renderer.mitadAltoVisible();
    const float mitadAncho = mitadAlto *
        (static_cast<float>(opciones.ancho) / opciones.alto);

    Escena escena;
    crearEscena(escena, opciones.cantidadVacas, mitadAncho, mitadAlto,
                opciones.semilla);

    CampoEstrellas campoEstrellas;
    crearCampoEstrellas(campoEstrellas, opciones.cantidadEstrellas,
                        mitadAncho, mitadAlto,
                        opciones.semilla ^ 0x9E3779B9u);

    SDL_Log("Modelo '%s': %d vertices, %d triangulos",
            opciones.rutaModelo.c_str(), modelo.verticesRaw, modelo.triangulos);
    SDL_Log("Modelo OVNI: %d vertices, %d triangulos",
            modeloOvni.verticesRaw, modeloOvni.triangulos);
    SDL_Log("Escena: %d vacas, %d estrellas, modo %s, semilla %u",
            opciones.cantidadVacas, opciones.cantidadEstrellas,
            nombreModo(opciones), opciones.semilla);

    bool corriendo = true;
    Uint32 anterior = SDL_GetTicks();

    constexpr float VENTANA_FPS = 0.25f;
    float tiempoFPS = 0.0f;
    int cuadrosFPS = 0;
    float fps = 0.0f;

    // Cronometros de la ventana actual (se vacian cada VENTANA_FPS): separan
    // el costo de la simulacion (donde actua OpenMP) del costo de dibujar,
    // que es lo que domina el frame hoy. msSimActual alimenta el HUD.
    Cronometro cronometroSim;
    Cronometro cronometroRender;
    float msSimActual = 0.0f;
    float msRenderActual = 0.0f;

    while (corriendo) {
        corriendo = procesarEventos();

        const Uint32 ahora = SDL_GetTicks();
        float dt = (ahora - anterior) / 1000.0f;
        anterior = ahora;
        dt = std::min(dt, 0.1f);

        tiempoFPS += dt;
        ++cuadrosFPS;
        if (tiempoFPS >= VENTANA_FPS) {
            fps = cuadrosFPS / tiempoFPS;
            tiempoFPS = 0.0f;
            cuadrosFPS = 0;

            msSimActual = static_cast<float>(cronometroSim.promedioMs());
            msRenderActual = static_cast<float>(cronometroRender.promedioMs());
            cronometroSim.limpiar();
            cronometroRender.limpiar();

            char titulo[200];
            SDL_snprintf(titulo, sizeof(titulo),
                         "ZipZip Espacial - %d vacas - %d estrellas - "
                         "%.1f FPS - sim %.2f ms - render %.2f ms - %s - %d hilos",
                         opciones.cantidadVacas, opciones.cantidadEstrellas,
                         fps, msSimActual, msRenderActual,
                         nombreModo(opciones), hilos);
            SDL_SetWindowTitle(ventana, titulo);
            SDL_Log("%s", titulo);
        }

        cronometroSim.iniciar();
        actualizarEscena(escena, dt, usarOpenMP(opciones));
        actualizarCampoEstrellas(
            campoEstrellas, dt,
            usarOpenMP(opciones) && opciones.paralelizarEstrellas);
        cronometroSim.detener();

        cronometroRender.iniciar();
        renderer.dibujar(modelo, modeloOvni, escena, campoEstrellas, fps,
                          msSimActual, hilos);
        SDL_GL_SwapWindow(ventana);
        cronometroRender.detener();
    }

    renderer.liberarRecursos();
    SDL_GL_DeleteContext(contexto);
    SDL_DestroyWindow(ventana);
    SDL_Quit();
    return 0;
}
