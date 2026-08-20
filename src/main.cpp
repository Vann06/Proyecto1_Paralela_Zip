#include "zipzip/assets/obj_loader.h"
#include "zipzip/rendering/renderer.h"
#include "zipzip/simulation/scene.h"
#include "zipzip/simulation/starfield.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <algorithm>
#include <cstdlib>
#include <string>

namespace {

constexpr int ANCHO = 900;
constexpr int ALTO = 700;
constexpr int VACAS_POR_DEFECTO = 20;
constexpr int ESTRELLAS_POR_DEFECTO = 180;

struct Opciones {
    int cantidadVacas = VACAS_POR_DEFECTO;
    std::string rutaModelo = "assets/models/cow.obj";
};

Opciones leerArgumentos(int argc, char* argv[]) {
    Opciones opciones;
    for (int i = 1; i < argc; ++i) {
        char* fin = nullptr;
        const long cantidad = std::strtol(argv[i], &fin, 10);
        if (fin != argv[i] && *fin == '\0') {
            opciones.cantidadVacas = cantidad > 0
                ? static_cast<int>(cantidad)
                : 1;
        } else {
            opciones.rutaModelo = argv[i];
        }
    }
    return opciones;
}

bool procesarEventos(Renderer& renderer, bool& pausa, bool& vsync) {
    SDL_Event evento;
    while (SDL_PollEvent(&evento)) {
        if (evento.type == SDL_QUIT) return false;

        if (evento.type != SDL_KEYDOWN) continue;

        switch (evento.key.keysym.sym) {
            case SDLK_ESCAPE:
                return false;
            case SDLK_SPACE:
                pausa = !pausa;
                break;
            case SDLK_w:
                renderer.alternarWireframe();
                break;
            case SDLK_c:
                renderer.alternarCulling();
                break;
            case SDLK_v:
                vsync = !vsync;
                SDL_GL_SetSwapInterval(vsync ? 1 : 0);
                break;
            default:
                break;
        }
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    const Opciones opciones = leerArgumentos(argc, argv);

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
        ANCHO,
        ALTO,
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

    SDL_GL_SetSwapInterval(1);
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

    Renderer renderer(ANCHO, ALTO);
    renderer.inicializar();
    renderer.prepararModelo(modelo);

    const float mitadAlto = renderer.mitadAltoVisible();
    const float mitadAncho = mitadAlto * (static_cast<float>(ANCHO) / ALTO);

    Escena escena;
    crearEscena(escena, opciones.cantidadVacas, mitadAncho, mitadAlto);

    CampoEstrellas campoEstrellas;
    crearCampoEstrellas(campoEstrellas, ESTRELLAS_POR_DEFECTO,
                        mitadAncho, mitadAlto);

    SDL_Log("Modelo '%s': %d vertices, %d triangulos",
            opciones.rutaModelo.c_str(), modelo.verticesRaw, modelo.triangulos);
    SDL_Log("Escena: %d vacas y %d estrellas",
            opciones.cantidadVacas, ESTRELLAS_POR_DEFECTO);

    bool corriendo = true;
    bool pausa = false;
    bool vsync = true;
    Uint32 anterior = SDL_GetTicks();

    constexpr float VENTANA_FPS = 0.25f;
    float tiempoFPS = 0.0f;
    int cuadrosFPS = 0;
    float fps = 0.0f;

    while (corriendo) {
        corriendo = procesarEventos(renderer, pausa, vsync);

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

            char titulo[160];
            SDL_snprintf(titulo, sizeof(titulo),
                         "ZipZip Espacial - %d vacas - %d estrellas - %.1f FPS",
                         opciones.cantidadVacas, ESTRELLAS_POR_DEFECTO, fps);
            SDL_SetWindowTitle(ventana, titulo);
        }

        if (!pausa) {
            actualizarEscena(escena, dt);
            actualizarCampoEstrellas(campoEstrellas, dt);
        }

        renderer.dibujar(modelo, escena, campoEstrellas, fps);
        SDL_GL_SwapWindow(ventana);
    }

    renderer.liberarModelo();
    SDL_GL_DeleteContext(contexto);
    SDL_DestroyWindow(ventana);
    SDL_Quit();
    return 0;
}
