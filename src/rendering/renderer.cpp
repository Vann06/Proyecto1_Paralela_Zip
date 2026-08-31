#include "zipzip/rendering/renderer.h"

#include "zipzip/core/camara.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

using zipzip::DIST_CAM;
using zipzip::PI;

// Inclinacion de la "capa de suelo" (plataforma + vacas + OVNI) sobre el
// eje X. Sin esto la camara mira el semicirculo de frente y se ve como un
// abanico plano; con la rotacion, la parte de arriba de la cupula se aleja
// de la camara y el semicirculo se ve escorzado, como el horizonte de un
// planeta visto desde un angulo. Se aplica antes de posicionar cada vaca/
// el OVNI (no despues) para que el giro afecte tambien donde caen, no solo
// como se ven.
constexpr float INCLINACION_SUELO = -55.0f;

// El OVNI esta mas cerca de la camara que las vacas (ver Ovni::z en
// scene.h), y un objeto cercano exagera visualmente cualquier rotacion
// heredada por perspectiva -- con la inclinacion completa del suelo se veia
// mas "acostado" que la plataforma misma. Esta fraccion es cuanto de esa
// inclinacion conserva su propio cuerpo (el resto se contrarresta en
// dibujarOvni); su posicion sobre la cupula si seguirla al 100%.
constexpr float INCLINACION_OVNI_FRACCION = 0.35f;

// Resolucion de la esfera de los planetas (usada al precomputar su
// geometria una sola vez y al dibujarla cada frame).
constexpr int SEGMENTOS_ESFERA = 32;
constexpr int LATITUDES_ESFERA = 16;

// Carga un BMP usando SDL2 y lo convierte a RGBA antes de subirlo a OpenGL.
// Devuelve false sin interrumpir el programa: quien dibuja conserva su color
// solido anterior como respaldo cuando el archivo falta o esta danado.
bool cargarTexturaBMP(const char* ruta, unsigned int& textura,
                      bool repetir) {
    SDL_Surface* original = SDL_LoadBMP(ruta);
    if (!original) {
        SDL_Log("Aviso: no se pudo cargar la textura '%s': %s",
                ruta, SDL_GetError());
        return false;
    }

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(
        original, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(original);
    if (!rgba) {
        SDL_Log("Aviso: no se pudo convertir la textura '%s': %s",
                ruta, SDL_GetError());
        return false;
    }

    glGenTextures(1, &textura);
    glBindTexture(GL_TEXTURE_2D, textura);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    repetir ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    repetir ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgba);

    SDL_Log("Textura cargada: %s", ruta);
    return true;
}

struct Glifo {
    char caracter;
    unsigned char fila[7];
};

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
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'I', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F } },
    { 'M', { 0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11 } },
    { 'N', { 0x11, 0x19, 0x15, 0x15, 0x13, 0x11, 0x11 } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
};

const unsigned char* buscarGlifo(char caracter) {
    for (const Glifo& glifo : FUENTE) {
        if (glifo.caracter == caracter) return glifo.fila;
    }
    return nullptr;
}

void dibujarTexto(const char* texto, float x, float y, float punto) {
    glBegin(GL_QUADS);
    float cursor = x;

    for (const char* p = texto; *p; ++p) {
        const unsigned char* filas = buscarGlifo(*p);
        if (filas) {
            for (int fila = 0; fila < 7; ++fila) {
                for (int columna = 0; columna < 5; ++columna) {
                    if (!(filas[fila] & (1 << (4 - columna)))) continue;
                    const float px = cursor + columna * punto;
                    const float py = y + fila * punto;
                    glVertex2f(px, py);
                    glVertex2f(px + punto, py);
                    glVertex2f(px + punto, py + punto);
                    glVertex2f(px, py + punto);
                }
            }
        }
        cursor += 6.0f * punto;
    }

    glEnd();
}

unsigned char convertirCanal(float valor) {
    const float ajustado = std::clamp(valor, 0.0f, 1.0f);
    return static_cast<unsigned char>(ajustado * 255.0f + 0.5f);
}

float transicionSuave(float borde0, float borde1, float valor) {
    const float t = std::clamp(
        (valor - borde0) / (borde1 - borde0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

std::vector<unsigned char> generarTexturaPlaneta(int estilo) {
    constexpr int ANCHO_TEXTURA = 96;
    constexpr int ALTO_TEXTURA = 48;

    const int indice = std::clamp(estilo, 0, 3);
    std::vector<unsigned char> pixeles(
        static_cast<size_t>(ANCHO_TEXTURA * ALTO_TEXTURA * 3));

    // Esta preparacion pertenece al modulo grafico y se mantiene en el hilo
    // principal. OpenMP se reserva para la simulacion, no para SDL/OpenGL ni
    // para la construccion de sus recursos.
    for (int y = 0; y < ALTO_TEXTURA; ++y) {
        const float v = (static_cast<float>(y) + 0.5f) /
                        static_cast<float>(ALTO_TEXTURA);
        for (int x = 0; x < ANCHO_TEXTURA; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) /
                            static_cast<float>(ANCHO_TEXTURA);
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (indice == 0) {
                // Mundo nebuloso: remolinos violetas con filamentos turquesa.
                const float remolino = 0.5f + 0.5f * std::sin(
                    2.0f * PI * (v * 5.0f +
                    0.20f * std::sin(2.0f * PI * u * 2.0f) +
                    0.05f * std::sin(2.0f * PI * (u * 7.0f - v * 3.0f))));
                const float filamento = std::pow(std::max(
                    0.0f, std::sin(2.0f * PI * (u * 3.0f + v * 4.0f))),
                    6.0f);
                r = 0.10f + 0.58f * remolino + 0.12f * filamento;
                g = 0.03f + 0.18f * remolino + 0.55f * filamento;
                b = 0.22f + 0.68f * remolino;
            } else if (indice == 1) {
                // Mundo oceanico: continentes, nubes y casquetes polares.
                const float relieve =
                    0.50f + 0.22f * std::sin(2.0f * PI * u * 2.0f) +
                    0.18f * std::cos(2.0f * PI * (u * 5.0f + v * 3.0f)) +
                    0.12f * std::sin(2.0f * PI * (u * 7.0f - v * 2.0f));
                const float tierra = transicionSuave(0.48f, 0.62f, relieve);
                const float nubes = 0.55f * std::pow(std::max(
                    0.0f,
                    std::sin(2.0f * PI * (u * 4.0f + v * 3.0f +
                    0.10f * std::sin(2.0f * PI * v * 5.0f)))), 8.0f);
                const float hielo = transicionSuave(
                    0.72f, 0.94f, std::fabs(2.0f * v - 1.0f));
                r = 0.02f + 0.10f * (1.0f - tierra) + 0.26f * tierra;
                g = 0.16f + 0.35f * (1.0f - tierra) + 0.38f * tierra;
                b = 0.34f + 0.48f * (1.0f - tierra) - 0.22f * tierra;
                r += nubes + 0.75f * hielo;
                g += nubes + 0.78f * hielo;
                b += nubes + 0.82f * hielo;
            } else if (indice == 2) {
                // Mundo volcanico: corteza oscura atravesada por lava.
                const float grietaA = std::fabs(std::sin(
                    2.0f * PI * (u * 3.0f +
                    0.18f * std::sin(2.0f * PI * v * 2.0f))));
                const float grietaB = std::fabs(std::cos(
                    2.0f * PI * (v * 4.0f +
                    0.14f * std::sin(2.0f * PI * u * 3.0f))));
                const float lava = 1.0f - std::clamp(
                    std::min(grietaA, grietaB) * 13.0f, 0.0f, 1.0f);
                const float roca = 0.5f + 0.5f * std::sin(
                    2.0f * PI * (u * 6.0f + v * 5.0f));
                r = 0.09f + 0.20f * roca + 0.91f * lava;
                g = 0.025f + 0.045f * roca + 0.48f * lava;
                b = 0.015f + 0.025f * roca + 0.04f * lava;
            } else {
                // Gigante gaseoso: bandas verdes y una tormenta brillante.
                const float bandas = 0.5f + 0.5f * std::sin(
                    2.0f * PI * (v * 11.0f +
                    0.09f * std::sin(2.0f * PI * u * 3.0f)));
                const float distanciaU = std::min(
                    std::fabs(u - 0.68f), 1.0f - std::fabs(u - 0.68f));
                const float distanciaV = v - 0.58f;
                const float tormenta = std::exp(
                    -(distanciaU * distanciaU / 0.010f +
                      distanciaV * distanciaV / 0.0035f));
                r = 0.03f + 0.17f * bandas + 0.62f * tormenta;
                g = 0.18f + 0.55f * bandas + 0.28f * tormenta;
                b = 0.16f + 0.32f * bandas + 0.20f * tormenta;
            }

            const float iluminacionPolar =
                0.78f + 0.22f * std::sin(PI * v);

            const size_t posicion = static_cast<size_t>(
                (y * ANCHO_TEXTURA + x) * 3);
            pixeles[posicion] = convertirCanal(r * iluminacionPolar);
            pixeles[posicion + 1u] = convertirCanal(g * iluminacionPolar);
            pixeles[posicion + 2u] = convertirCanal(b * iluminacionPolar);
        }
    }

    return pixeles;
}

} // namespace

Renderer::Renderer(int ancho, int alto) : ancho_(ancho), alto_(alto) {}

void Renderer::inicializar() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    const GLfloat ambiente[] = { 0.25f, 0.25f, 0.28f, 1.0f };
    const GLfloat difusa[] = { 0.90f, 0.90f, 0.85f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, difusa);

    // Las arrays de cliente son una capacidad de OpenGL, no un enlace a un
    // buffer en particular: se habilitan una vez y cada dibujarX() fija sus
    // propios punteros justo antes de dibujar, lo que permite alternar entre
    // varios modelos (vacas, OVNI) en el mismo frame.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    prepararTexturasEscena();
    prepararTexturasPlanetas();
    prepararEsferaPlaneta();
    configurarProyeccion();
}

void Renderer::prepararTexturasEscena() {
    cargarTexturaBMP("assets/textures/space_background.bmp",
                     texturaFondo_, false);
    cargarTexturaBMP("assets/textures/grass.bmp", texturaGrama_, true);
}

void Renderer::prepararEsferaPlaneta() {
    // La misma esfera unitaria sirve para los cuatro planetas (solo cambian
    // escala, textura y rotacion, que se aplican con glScalef/glRotatef).
    // Antes esto se recalculaba con seno/coseno para cada planeta en cada
    // frame; aqui se calcula una sola vez y dibujarPlanetas solo reproduce
    // los valores guardados.
    esferaPlaneta_.clear();
    esferaPlaneta_.reserve(static_cast<size_t>(
        SEGMENTOS_ESFERA * LATITUDES_ESFERA * 4 * 5));

    // u/v son las coordenadas de textura (0..1); theta/phi son los angulos
    // esfericos que u/v generan (longitud y colatitud). No confundir estas
    // u,v con las de crearEscena en scene.cpp, que son otra cosa (muestras
    // uniformes para repartir vacas en el rombo).
    for (int indiceSegmento = 0; indiceSegmento < SEGMENTOS_ESFERA;
         ++indiceSegmento) {
        const float u1 = static_cast<float>(indiceSegmento) / SEGMENTOS_ESFERA;
        const float u2 =
            static_cast<float>(indiceSegmento + 1) / SEGMENTOS_ESFERA;
        const float theta1 = 2.0f * PI * u1;
        const float theta2 = 2.0f * PI * u2;

        for (int indiceLatitud = 0; indiceLatitud < LATITUDES_ESFERA;
             ++indiceLatitud) {
            const float v1 = static_cast<float>(indiceLatitud) / LATITUDES_ESFERA;
            const float v2 =
                static_cast<float>(indiceLatitud + 1) / LATITUDES_ESFERA;
            const float phi1 = PI * v1;
            const float phi2 = PI * v2;
            const float radio1 = std::sin(phi1);
            const float radio2 = std::sin(phi2);

            const float vertices[4][5] = {
                { u1, v1, radio1 * std::cos(theta1), std::cos(phi1),
                  radio1 * std::sin(theta1) },
                { u2, v1, radio1 * std::cos(theta2), std::cos(phi1),
                  radio1 * std::sin(theta2) },
                { u2, v2, radio2 * std::cos(theta2), std::cos(phi2),
                  radio2 * std::sin(theta2) },
                { u1, v2, radio2 * std::cos(theta1), std::cos(phi2),
                  radio2 * std::sin(theta1) },
            };
            for (const auto& vert : vertices) {
                esferaPlaneta_.insert(esferaPlaneta_.end(),
                                      vert, vert + 5);
            }
        }
    }
}

void Renderer::prepararTexturasPlanetas() {
    if (texturasPlanetasPreparadas_) return;

    constexpr int ANCHO_TEXTURA = 96;
    constexpr int ALTO_TEXTURA = 48;
    glGenTextures(4, texturasPlanetas_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < 4; ++i) {
        const std::vector<unsigned char> pixeles = generarTexturaPlaneta(i);
        glBindTexture(GL_TEXTURE_2D, texturasPlanetas_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     ANCHO_TEXTURA, ALTO_TEXTURA, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, pixeles.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    texturasPlanetasPreparadas_ = true;
}

void Renderer::configurarProyeccion() {
    glViewport(0, 0, ancho_, alto_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const float fov = zipzip::FOV_GRAD * PI / 180.0f;
    const float aspecto = static_cast<float>(ancho_) / alto_;
    const float zCerca = 0.1f;
    const float zLejos = 100.0f;
    const float arriba = zCerca * std::tan(fov * 0.5f);
    const float derecha = arriba * aspecto;
    glFrustum(-derecha, derecha, -arriba, arriba, zCerca, zLejos);

    glMatrixMode(GL_MODELVIEW);
}

float Renderer::mitadAltoVisible() const {
    return zipzip::mitadAltoVisible();
}

void Renderer::liberarRecursos() {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    if (texturasPlanetasPreparadas_) {
        glDeleteTextures(4, texturasPlanetas_);
        std::fill(std::begin(texturasPlanetas_),
                  std::end(texturasPlanetas_), 0u);
        texturasPlanetasPreparadas_ = false;
    }
    if (texturaFondo_ != 0) {
        glDeleteTextures(1, &texturaFondo_);
        texturaFondo_ = 0;
    }
    if (texturaGrama_ != 0) {
        glDeleteTextures(1, &texturaGrama_);
        texturaGrama_ = 0;
    }
}

void Renderer::restaurarEstadoRender() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    if (culling_) glEnable(GL_CULL_FACE);
    if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void Renderer::alternarWireframe() {
    wireframe_ = !wireframe_;
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);
}

void Renderer::alternarCulling() {
    culling_ = !culling_;
    if (culling_) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void Renderer::dibujarFondo() {
    if (texturaFondo_ == 0) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, ancho_, alto_, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaFondo_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(static_cast<float>(ancho_), 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(static_cast<float>(ancho_),
                                         static_cast<float>(alto_));
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f,
                                         static_cast<float>(alto_));
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::dibujarEstrellas(const CampoEstrellas& campo) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    dibujarFondo();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);

    // glPointSize solo puede cambiar fuera de glBegin/glEnd (el color si
    // puede variar dentro), asi que agrupar por tamano redondeado reduce el
    // par glBegin/glEnd de una vez por estrella a unos pocos lotes.
    constexpr float PASO_TAMANO = 0.35f;
    std::vector<size_t> ordenPorTamano(campo.estrellas.size());
    for (size_t i = 0; i < ordenPorTamano.size(); ++i) ordenPorTamano[i] = i;
    std::sort(ordenPorTamano.begin(), ordenPorTamano.end(),
              [&campo](size_t a, size_t b) {
                  return campo.estrellas[a].tamano < campo.estrellas[b].tamano;
              });

    bool loteAbierto = false;
    float baldeActual = -1.0f;
    for (size_t indice : ordenPorTamano) {
        const Estrella& estrella = campo.estrellas[indice];
        const float balde = std::floor(estrella.tamano / PASO_TAMANO);
        if (!loteAbierto || balde != baldeActual) {
            if (loteAbierto) glEnd();
            baldeActual = balde;
            glPointSize(std::max(1.0f, balde * PASO_TAMANO));
            glBegin(GL_POINTS);
            loteAbierto = true;
        }
        glColor4f(0.72f * estrella.brillo,
                  0.84f * estrella.brillo,
                  estrella.brillo,
                  estrella.brillo);
        glVertex3f(estrella.x, estrella.y, -1.0f);
    }
    if (loteAbierto) glEnd();

    for (const EstrellaFugaz& fugaz : campo.fugaces) {
        if (!fugaz.activa || fugaz.brillo <= 0.0f) continue;

        const float rapidez = std::sqrt(
            fugaz.vx * fugaz.vx + fugaz.vy * fugaz.vy);
        if (rapidez <= 0.0f) continue;

        const float colaX = fugaz.x -
            (fugaz.vx / rapidez) * fugaz.longitud;
        const float colaY = fugaz.y -
            (fugaz.vy / rapidez) * fugaz.longitud;

        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glColor4f(0.45f, 0.70f, 1.0f, 0.20f * fugaz.brillo);
        glVertex3f(colaX, colaY, -0.95f);
        glColor4f(0.80f, 0.92f, 1.0f, 0.55f * fugaz.brillo);
        glVertex3f(fugaz.x, fugaz.y, -0.95f);
        glEnd();

        glLineWidth(1.5f);
        glBegin(GL_LINES);
        glColor4f(0.55f, 0.80f, 1.0f, 0.0f);
        glVertex3f(colaX, colaY, -0.94f);
        glColor4f(1.0f, 1.0f, 1.0f, fugaz.brillo);
        glVertex3f(fugaz.x, fugaz.y, -0.94f);
        glEnd();
    }

    glPointSize(1.0f);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    restaurarEstadoRender();
}

void Renderer::dibujarPlataforma(const Escena& escena) {
    glDisable(GL_LIGHTING);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);
    glRotatef(INCLINACION_SUELO, 1.0f, 0.0f, 0.0f);

    // Semicirculo apoyado en el borde inferior de la pantalla, como el
    // horizonte curvo de un pequeño planeta. 'caidaZ' es cuanto se extiende
    // el borde curvo hacia atras EN PROFUNDIDAD (no en Y) para que la
    // plataforma se vea como un bloque solido y no como una cascara plana.
    // Tiene que ser en Z: si se extendiera en Y (como antes), esa franja
    // cae dentro del mismo rango de Y que ya ocupa el abanico superior para
    // otros angulos del arco, y despues de INCLINACION_SUELO la falda
    // termina superponiendose con la cara superior en vez de quedar
    // escondida detras.
    const float radio = escena.circuloRadio;
    const float centroY = escena.circuloCentroY;
    const float caidaZ = escena.mitadAlto * 0.7f;

    constexpr int SEGMENTOS_PLATAFORMA = 48;

    // z = 0 para toda la cara superior: el mismo plano exacto donde
    // dibujarVacas() coloca a cada vaca (glTranslatef(vaca.x, vaca.y,
    // 0.0f)). Antes esta funcion le sumaba una profundidad falsa por altura
    // para simular perspectiva "a mano" -- pero ahora que INCLINACION_SUELO
    // ya gira de verdad toda la capa de suelo, esa Z extra hacia que el
    // circulo dibujado ya no fuera el mismo circulo que usa la fisica (ver
    // circuloRadio/circuloCentroY), y las vacas cerca del borde quedaban
    // renderizadas fuera de la plataforma. Con z = 0 en los dos, coinciden
    // exactamente.
    auto puntoArco = [&](int indice) {
        const float angulo = PI * static_cast<float>(indice) /
                             static_cast<float>(SEGMENTOS_PLATAFORMA);
        const float x = radio * std::cos(angulo);
        const float y = centroY + radio * std::sin(angulo);
        return std::array<float, 2>{x, y};
    };

    // ============================
    // CARA SUPERIOR (abanico de triangulos desde el centro)
    // ============================
    const bool usarTexturaGrama = texturaGrama_ != 0;
    if (usarTexturaGrama) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaGrama_);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glBegin(GL_TRIANGLE_FAN);
    if (usarTexturaGrama) glColor3f(0.82f, 0.90f, 0.78f);
    else glColor3f(0.62f, 0.90f, 0.10f);
    glTexCoord2f(2.5f, 0.0f);
    glVertex3f(0.0f, centroY, 0.0f);
    if (usarTexturaGrama) glColor3f(0.92f, 0.98f, 0.88f);
    else glColor3f(0.72f, 0.95f, 0.08f);
    for (int i = 0; i <= SEGMENTOS_PLATAFORMA; ++i) {
        const auto p = puntoArco(i);
        const float u = (p[0] / radio + 1.0f) * 2.5f;
        const float v = ((p[1] - centroY) / radio) * 5.0f;
        glTexCoord2f(u, v);
        glVertex3f(p[0], p[1], 0.0f);
    }
    glEnd();

    if (usarTexturaGrama) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    // ============================
    // FALDA CURVA (pared que baja desde el arco)
    // ============================
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= SEGMENTOS_PLATAFORMA; ++i) {
        const auto p = puntoArco(i);
        glColor3f(0.45f, 0.72f, 0.09f);
        glVertex3f(p[0], p[1], 0.0f);
        glColor3f(0.28f, 0.50f, 0.10f);
        glVertex3f(p[0], p[1], -caidaZ);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void Renderer::dibujarVacas(const Modelo& modelo, const Escena& escena) {
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);
    glRotatef(INCLINACION_SUELO, 1.0f, 0.0f, 0.0f);
    const GLfloat posicionLuz[] = { 2.0f, 3.0f, 4.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, posicionLuz);

    // El puntero se fija aqui, no en inicializar(): asi cada lote de
    // instancias puede usar un modelo distinto (vacas, y mas adelante OVNI)
    // dentro del mismo frame.
    glVertexPointer(3, GL_FLOAT, 0, modelo.pos.data());
    glNormalPointer(GL_FLOAT, 0, modelo.nrm.data());

    const GLsizei vertices = modelo.triangulos * 3;
    for (const Instancia& vaca : escena.vacas) {
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -DIST_CAM);
        // La inclinacion va antes del traslado a (vaca.x, vaca.y): asi la
        // vaca cae en el punto correcto de la cupula inclinada, no solo se
        // ve inclinada parada en su posicion plana original.
        glRotatef(INCLINACION_SUELO, 1.0f, 0.0f, 0.0f);
        glTranslatef(vaca.x, vaca.y, 0.0f);
        glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(vaca.giro, 0.0f, 1.0f, 0.0f);
        glScalef(vaca.escala, vaca.escala, vaca.escala);
        glColor3f(vaca.r, vaca.g, vaca.b);
        glDrawArrays(GL_TRIANGLES, 0, vertices);
    }
}

void Renderer::dibujarOvni(const Modelo& modeloOvni, const Escena& escena) {
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);
    glRotatef(INCLINACION_SUELO, 1.0f, 0.0f, 0.0f);
    glTranslatef(escena.ovni.x, escena.ovni.y, escena.ovni.z);
    // Contrarresta la mayor parte de la inclinacion heredada: la posicion
    // ya quedo fijada sobre la cupula inclinada (arriba), pero el cuerpo
    // del OVNI se dibuja casi de pie -- solo conserva
    // INCLINACION_OVNI_FRACCION del angulo del suelo.
    glRotatef(-INCLINACION_SUELO * (1.0f - INCLINACION_OVNI_FRACCION),
              1.0f, 0.0f, 0.0f);
    glRotatef(escena.ovni.giro, 0.0f, 1.0f, 0.0f);
    glScalef(escena.ovni.escala, escena.ovni.escala, escena.ovni.escala);

    glVertexPointer(3, GL_FLOAT, 0, modeloOvni.pos.data());
    glNormalPointer(GL_FLOAT, 0, modeloOvni.nrm.data());

    const bool tieneGrupoGato = std::any_of(
        modeloOvni.rangos.begin(), modeloOvni.rangos.end(),
        [](const RangoModelo& rango) { return rango.nombre == "gato"; });

    if (!tieneGrupoGato) {
        // Compatibilidad con cualquier OBJ anterior que no tenga grupos.
        glColor3f(0.20f, 0.75f, 0.35f);
        glDrawArrays(GL_TRIANGLES, 0, modeloOvni.triangulos * 3);
        return;
    }

    const GLfloat sinEmision[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat reflejoMetal[] = { 0.92f, 0.95f, 1.0f, 1.0f };
    const GLfloat emisionNeon[] = { 0.02f, 0.42f, 0.08f, 1.0f };

    for (const RangoModelo& rango : modeloOvni.rangos) {
        if (rango.nombre == "gato") {
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sinEmision);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emisionNeon);
            glColor3f(0.10f, 1.0f, 0.25f);
        } else {
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, sinEmision);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, reflejoMetal);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 88.0f);
            glColor3f(0.48f, 0.53f, 0.62f);
        }
        glDrawArrays(GL_TRIANGLES, rango.primerVertice,
                     rango.cantidadVertices);
    }

    // No dejar emision ni brillo metalico activos para el siguiente frame.
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, sinEmision);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sinEmision);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}

void Renderer::dibujarHUD(float fps, int vacas, int estrellas,
                           float msSim, int hilos, float distMinOvni) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, ancho_, alto_, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.01f, 0.02f, 0.08f, 0.72f);
    glBegin(GL_QUADS);
    glVertex2f(8.0f, 8.0f);
    glVertex2f(240.0f, 8.0f);
    glVertex2f(240.0f, 132.0f);
    glVertex2f(8.0f, 132.0f);
    glEnd();
    glDisable(GL_BLEND);

    char linea[64];
    glColor3f(0.55f, 1.0f, 0.65f);
    std::snprintf(linea, sizeof(linea), "FPS: %.1f", fps);
    dibujarTexto(linea, 18.0f, 16.0f, 3.0f);

    glColor3f(0.75f, 0.78f, 0.85f);
    std::snprintf(linea, sizeof(linea), "VACAS: %d", vacas);
    dibujarTexto(linea, 18.0f, 46.0f, 2.0f);

    glColor3f(0.65f, 0.76f, 1.0f);
    std::snprintf(linea, sizeof(linea), "EST: %d", estrellas);
    dibujarTexto(linea, 18.0f, 68.0f, 2.0f);

    // Tiempo de la simulacion (CPU) por separado del tiempo total de frame:
    // en esta parte es donde OpenMP actua, y donde debe verse el speedup.
    glColor3f(1.0f, 0.82f, 0.45f);
    std::snprintf(linea, sizeof(linea), "SIM: %.2f MS N%d", msSim, hilos);
    dibujarTexto(linea, 18.0f, 90.0f, 2.0f);

    // Distancia minima al OVNI: calculada con una reduccion dentro de la
    // misma pasada O(N^2), no un lazo aparte.
    glColor3f(1.0f, 0.55f, 0.55f);
    std::snprintf(linea, sizeof(linea), "MIN: %.2f", distMinOvni);
    dibujarTexto(linea, 18.0f, 112.0f, 2.0f);

    restaurarEstadoRender();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::dibujar(const Modelo& modeloVaca, const Modelo& modeloOvni,
                        const Escena& escena, const CampoEstrellas& campo,
                        float fps, float msSim, int hilos) {
    glClearColor(0.008f, 0.012f, 0.045f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    dibujarEstrellas(campo);
    dibujarPlanetas(escena);
    dibujarPlataforma(escena);
    dibujarVacas(modeloVaca, escena);
    dibujarOvni(modeloOvni, escena);
    dibujarHUD(fps, static_cast<int>(escena.vacas.size()),
               static_cast<int>(campo.estrellas.size()), msSim, hilos,
               escena.distanciaMinimaOvni);
}

void Renderer::dibujarPlanetas(const Escena& escena) {
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);

    constexpr int SEGMENTOS_ARO = 64;

    for (const Planeta& planeta : escena.planetas) {
        glPushMatrix();
        glTranslatef(planeta.x, planeta.y, -2.0f);
        glScalef(planeta.escala, planeta.escala, planeta.escala);

        const int indiceTextura = std::clamp(planeta.textura, 0, 3);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturasPlanetas_[indiceTextura]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.98f);

        // La esfera gira sobre su eje Y; al rotar la geometria también rota
        // el mapa UV y el movimiento de la textura se vuelve visible.
        glPushMatrix();
        glRotatef(planeta.giro, 0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        // Reproduce el buffer precomputado en prepararEsferaPlaneta(): cada
        // vertice ocupa 5 floats consecutivos (u, v, x, y, z).
        for (size_t indiceVertice = 0;
             indiceVertice + 4 < esferaPlaneta_.size(); indiceVertice += 5) {
            glTexCoord2f(esferaPlaneta_[indiceVertice],
                         esferaPlaneta_[indiceVertice + 1]);
            glVertex3f(esferaPlaneta_[indiceVertice + 2],
                       esferaPlaneta_[indiceVertice + 3],
                       esferaPlaneta_[indiceVertice + 4]);
        }
        glEnd();
        glPopMatrix();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        if (planeta.tieneAro) {
            // Solo dos planetas poseen aro. La prueba de profundidad oculta
            // su mitad posterior cuando pasa detras de la esfera.
            glRotatef(planeta.anguloAroInicial, 0.0f, 0.0f, 1.0f);
            glRotatef(planeta.inclinacionAro, 1.0f, 0.0f, 0.0f);
            glBegin(GL_QUAD_STRIP);
            for (int indiceSegmentoAro = 0; indiceSegmentoAro <= SEGMENTOS_ARO;
                 ++indiceSegmentoAro) {
                const float angulo = 2.0f * PI *
                    static_cast<float>(indiceSegmentoAro) /
                    static_cast<float>(SEGMENTOS_ARO);
                const float coseno = std::cos(angulo);
                const float seno = std::sin(angulo);
                const float pulso =
                    0.72f + 0.28f * std::sin(angulo * 5.0f);

                glColor4f(planeta.r * pulso, planeta.g * pulso,
                          planeta.b * pulso, 0.18f);
                glVertex3f(1.18f * coseno, 1.18f * seno, 0.0f);
                glColor4f(std::min(1.0f, planeta.r * 1.45f),
                          std::min(1.0f, planeta.g * 1.45f),
                          std::min(1.0f, planeta.b * 1.45f), 0.62f);
                glVertex3f(1.72f * coseno, 1.72f * seno, 0.0f);
            }
            glEnd();
        }

        glPopMatrix();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    restaurarEstadoRender();
}
