// main.cpp
//
// Screensaver de vacas con SDL2 + OpenGL (pipeline fijo).
// La carga del archivo vive en obj_loader.cpp y el estado de las
// instancias en escena.cpp; aca solo hay ventana y render.
//
// Compilar:
//   g++ -O2 main.cpp obj_loader.cpp escena.cpp -o visor -lSDL2 -lGL
//
// Usar:
//   ./visor 50            50 vacas, modelo cow.obj
//   ./visor 200 cow.obj   el orden de los parametros no importa
//
// Controles:
//   ESPACIO  pausar
//   W        alternar wireframe
//   C        alternar face culling
//   V        alternar vsync (apagarlo deja correr los FPS libres)
//   ESC      salir

#include "obj_loader.h"
#include "escena.h"

#include <SDL2/SDL.h>
#include <GL/gl.h>

#include <cmath>
#include <cstdlib>
#include <string>

namespace {

const int   ANCHO     = 900;
const int   ALTO      = 700;
const float FOV_GRAD  = 60.0f;
const float DIST_CAM  = 4.0f;   // la escena vive en el plano z = 0
const int   VACAS_DEF = 20;

void configurarProyeccion(int ancho, int alto) {
    glViewport(0, 0, ancho, alto);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fov    = FOV_GRAD * 3.14159265f / 180.0f;
    float aspect = static_cast<float>(ancho) / static_cast<float>(alto);
    float znear  = 0.1f, zfar = 100.0f;
    float top    = znear * std::tan(fov * 0.5f);
    float right  = top * aspect;
    glFrustum(-right, right, -top, top, znear, zfar);

    glMatrixMode(GL_MODELVIEW);
}

void configurarEstadoGL() {
    glEnable(GL_DEPTH_TEST);

    // Culling apagado por defecto: muchos OBJ traen winding inconsistente
    // y se verian agujeros. Se activa con la tecla C.
    glDisable(GL_CULL_FACE);

    // Sin iluminacion un modelo de un solo color se ve como silueta plana.
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);   // necesario: el loader y cada instancia escalan
    glShadeModel(GL_SMOOTH);

    GLfloat ambiente[] = { 0.25f, 0.25f, 0.28f, 1.0f };
    GLfloat difusa[]   = { 0.90f, 0.90f, 0.85f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difusa);
}

// Mitad del alto visible en el plano donde viven las instancias.
float mitadAltoVisible() {
    float fov = FOV_GRAD * 3.14159265f / 180.0f;
    return DIST_CAM * std::tan(fov * 0.5f);
}

struct Opciones {
    int         cantidad = VACAS_DEF;
    std::string ruta     = "cow.obj";
};

// Los parametros se aceptan en cualquier orden: si es un numero es la
// cantidad de vacas, si no, es la ruta del modelo.
Opciones leerArgumentos(int argc, char* argv[]) {
    Opciones o;
    for (int i = 1; i < argc; ++i) {
        char* fin = nullptr;
        long n = std::strtol(argv[i], &fin, 10);
        if (fin != argv[i] && *fin == '\0') {
            o.cantidad = (n > 0) ? static_cast<int>(n) : 1;
        } else {
            o.ruta = argv[i];
        }
    }
    return o;
}

// Devuelve false cuando hay que cerrar la ventana.
bool procesarEventos(bool& pausa, bool& wireframe, bool& culling, bool& vsync) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;

        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    return false;
                case SDLK_SPACE:
                    pausa = !pausa;
                    break;
                case SDLK_w:
                    wireframe = !wireframe;
                    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
                    break;
                case SDLK_c:
                    culling = !culling;
                    if (culling) glEnable(GL_CULL_FACE);
                    else         glDisable(GL_CULL_FACE);
                    break;
                case SDLK_v:
                    // Con vsync los FPS quedan clavados en la tasa del
                    // monitor; apagarlo es lo que deja medir de verdad.
                    vsync = !vsync;
                    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
                    break;
                default:
                    break;
            }
        }
    }
    return true;
}

// ------------------------------------------------------------- texto en pantalla
//
// Fuente de mapa de bits de 5x7 hecha a mano. Es fea, pero evita depender de
// SDL_ttf y de tener que distribuir un archivo .ttf junto al programa.
// Cada glifo son 7 bytes, uno por fila, y solo importan los 5 bits bajos:
// el bit 4 es la columna izquierda y el bit 0 la derecha.

struct Glifo { char c; unsigned char fila[7]; };

const Glifo FUENTE[] = {
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
    { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
    { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
    { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
    { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
    { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
    { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } },
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
};

// Devuelve las 7 filas del caracter, o nullptr si no esta en la fuente
// (el espacio y cualquier otro simbolo simplemente avanzan el cursor).
const unsigned char* buscarGlifo(char c) {
    for (const Glifo& g : FUENTE) {
        if (g.c == c) return g.fila;
    }
    return nullptr;
}

// Dibuja 's' con la esquina superior izquierda en (x, y), en pixeles.
// 'punto' es el lado de cada pixel del glifo, asi que un caracter mide
// 5*punto de ancho por 7*punto de alto.
void dibujarTexto(const char* s, float x, float y, float punto) {
    glBegin(GL_QUADS);
    float cursor = x;

    for (const char* p = s; *p; ++p) {
        const unsigned char* filas = buscarGlifo(*p);
        if (filas) {
            for (int f = 0; f < 7; ++f) {
                for (int c = 0; c < 5; ++c) {
                    if (!(filas[f] & (1 << (4 - c)))) continue;
                    float px = cursor + c * punto;
                    float py = y + f * punto;
                    glVertex2f(px,         py);
                    glVertex2f(px + punto, py);
                    glVertex2f(px + punto, py + punto);
                    glVertex2f(px,         py + punto);
                }
            }
        }
        cursor += 6.0f * punto;   // 5 de ancho + 1 de separacion
    }

    glEnd();
}

// Panel con FPS y cantidad de vacas, encima de la escena.
//
// Se dibuja con una proyeccion ortografica en pixeles y sin iluminacion ni
// profundidad, para que el texto no compita con el modelo 3D. El wireframe y
// el culling tambien se apagan aca: si no, las letras saldrian como contornos
// huecos o directamente invisibles segun el estado en que quedo la escena.
void dibujarHUD(int ancho, int alto, float fps, int vacas,
                bool wireframe, bool culling) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, ancho, alto, 0.0, -1.0, 1.0);   // y crece hacia abajo

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Fondo oscuro semitransparente: sin esto el texto se pierde cuando pasa
    // una vaca clara por detras.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    // Ancho pensado para "FPS: 1234.5": sin vsync el contador llega a cuatro
    // digitos y con un panel angosto el texto se sale del fondo.
    glBegin(GL_QUADS);
    glVertex2f(8.0f,   8.0f);
    glVertex2f(240.0f, 8.0f);
    glVertex2f(240.0f, 66.0f);
    glVertex2f(8.0f,   66.0f);
    glEnd();
    glDisable(GL_BLEND);

    char linea[64];

    glColor3f(0.55f, 1.0f, 0.65f);
    SDL_snprintf(linea, sizeof(linea), "FPS: %.1f", fps);
    dibujarTexto(linea, 18.0f, 16.0f, 3.0f);

    glColor3f(0.75f, 0.78f, 0.85f);
    SDL_snprintf(linea, sizeof(linea), "VACAS: %d", vacas);
    dibujarTexto(linea, 18.0f, 46.0f, 2.0f);

    // Devolver el estado como estaba antes del panel.
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (culling)   glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void dibujar(const Modelo& m, const Escena& e) {
    glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // La luz se posiciona con la matriz de la camara y sin las transformadas
    // de las instancias, asi queda fija en el mundo y todas las vacas se
    // iluminan desde el mismo lado.
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);
    GLfloat posLuz[] = { 2.0f, 3.0f, 4.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, posLuz);

    const GLsizei vertices = m.triangulos * 3;

    for (const Instancia& v : e.vacas) {
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -DIST_CAM);
        glTranslatef(v.x, v.y, 0.0f);
        glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(v.giro, 0.0f, 1.0f, 0.0f);
        glScalef(v.escala, v.escala, v.escala);

        glColor3f(v.r, v.g, v.b);
        glDrawArrays(GL_TRIANGLES, 0, vertices);
    }
}

} // namespace anonimo

int main(int argc, char* argv[]) {
    const Opciones opciones = leerArgumentos(argc, argv);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Screensaver de vacas",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          ANCHO, ALTO,
                                          SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("SDL_GL_CreateContext: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    SDL_Log("Version:  %s", glGetString(GL_VERSION));
    SDL_Log("Renderer: %s", glGetString(GL_RENDERER));

    // --- carga del modelo, delegada al modulo
    Modelo modelo;
    std::string error;
    if (!cargarOBJ(opciones.ruta, modelo, OpcionesOBJ(), &error)) {
        SDL_Log("Error cargando modelo: %s", error.c_str());
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Log("Cargado '%s': %d vertices unicos, %d triangulos",
            opciones.ruta.c_str(), modelo.verticesRaw, modelo.triangulos);

    configurarEstadoGL();
    configurarProyeccion(ANCHO, ALTO);

    // --- escena: n instancias repartidas sobre el area visible
    float mitadAlto  = mitadAltoVisible();
    float mitadAncho = mitadAlto * (static_cast<float>(ANCHO) / ALTO);

    Escena escena;
    crearEscena(escena, opciones.cantidad, mitadAncho, mitadAlto);

    SDL_Log("Vacas: %d | triangulos por cuadro: %d",
            opciones.cantidad, modelo.triangulos * opciones.cantidad);

    // Los punteros apuntan a los vectores del modelo, que viven hasta el
    // final de main. Si el modelo se recargara habria que volver a llamar
    // glVertexPointer, porque los datos podrian moverse en memoria.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, modelo.pos.data());
    glNormalPointer(GL_FLOAT, 0, modelo.nrm.data());

    bool corriendo = true;
    bool pausa     = false;
    bool wireframe = false;
    bool culling   = false;
    bool vsync     = true;

    Uint32 anterior = SDL_GetTicks();

    // El contador se promedia sobre una ventana de tiempo en vez de usar el
    // dt de un solo cuadro: asi el numero no salta en cada frame y se puede
    // leer. Media ventana de 0.25 s da un refresco de 4 veces por segundo.
    const float VENTANA_FPS = 0.25f;
    float acumTiempo  = 0.0f;
    int   acumCuadros = 0;
    float fps         = 0.0f;

    while (corriendo) {
        corriendo = procesarEventos(pausa, wireframe, culling, vsync);

        Uint32 ahora = SDL_GetTicks();
        float dt = (ahora - anterior) / 1000.0f;
        anterior = ahora;

        acumTiempo += dt;
        ++acumCuadros;
        if (acumTiempo >= VENTANA_FPS) {
            fps = acumCuadros / acumTiempo;
            acumTiempo  = 0.0f;
            acumCuadros = 0;

            char titulo[128];
            SDL_snprintf(titulo, sizeof(titulo),
                         "Screensaver de vacas  -  %d vacas  -  %.1f FPS",
                         opciones.cantidad, fps);
            SDL_SetWindowTitle(window, titulo);
        }

        if (!pausa) actualizarEscena(escena, dt);

        dibujar(modelo, escena);
        dibujarHUD(ANCHO, ALTO, fps, opciones.cantidad, wireframe, culling);
        SDL_GL_SwapWindow(window);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
