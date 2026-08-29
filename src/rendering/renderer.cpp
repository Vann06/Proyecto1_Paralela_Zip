#include "zipzip/rendering/renderer.h"

#include <SDL2/SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr float FOV_GRAD = 60.0f;
constexpr float DIST_CAM = 4.0f;
constexpr float PI = 3.14159265358979323846f;

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

    prepararTexturasPlanetas();
    configurarProyeccion();
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

    const float fov = FOV_GRAD * PI / 180.0f;
    const float aspecto = static_cast<float>(ancho_) / alto_;
    const float zCerca = 0.1f;
    const float zLejos = 100.0f;
    const float arriba = zCerca * std::tan(fov * 0.5f);
    const float derecha = arriba * aspecto;
    glFrustum(-derecha, derecha, -arriba, arriba, zCerca, zLejos);

    glMatrixMode(GL_MODELVIEW);
}

float Renderer::mitadAltoVisible() const {
    const float fov = FOV_GRAD * PI / 180.0f;
    return DIST_CAM * std::tan(fov * 0.5f);
}

void Renderer::prepararModelo(const Modelo& modelo) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, modelo.pos.data());
    glNormalPointer(GL_FLOAT, 0, modelo.nrm.data());
}

void Renderer::liberarModelo() {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    if (texturasPlanetasPreparadas_) {
        glDeleteTextures(4, texturasPlanetas_);
        std::fill(std::begin(texturasPlanetas_),
                  std::end(texturasPlanetas_), 0u);
        texturasPlanetasPreparadas_ = false;
    }
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

void Renderer::dibujarEstrellas(const CampoEstrellas& campo) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);

    for (const Estrella& estrella : campo.estrellas) {
        glPointSize(estrella.tamano);
        glColor4f(0.72f * estrella.brillo,
                  0.84f * estrella.brillo,
                  estrella.brillo,
                  estrella.brillo);
        glBegin(GL_POINTS);
        glVertex3f(estrella.x, estrella.y, -1.0f);
        glEnd();
    }

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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    if (culling_) glEnable(GL_CULL_FACE);
    if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void Renderer::dibujarPlataforma(const Escena& escena) {
    glDisable(GL_LIGHTING);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);

    // Tamaño general del bloque
    const float ancho = escena.romboAncho;
    const float alto  = escena.romboAlto;
    const float caida = escena.mitadAlto  * 0.98f;

    // ============================
    // VERTICES DE LA CARA SUPERIOR
    // (rombo / punta hacia arriba)
    // ============================

    const float xTop    = 0.0f;
    const float yTop    =  alto;
    const float zTop    = -0.68f;

    const float xRight  =  ancho;
    const float yRight  =  0.0f;
    const float zRight  = -0.42f;

    const float xBottom =  0.0f;
    const float yBottom = -alto;
    const float zBottom = -0.18f;

    const float xLeft   = -ancho;
    const float yLeft   =  0.0f;
    const float zLeft   = -0.42f;

    // ============================
    // VERTICES INFERIORES
    // para las caras laterales
    // ============================

    const float xLowerRight  =  ancho;
    const float yLowerRight  = -caida;
    const float zLowerRight  = -0.42f;

    const float xLowerBottom =  0.0f;
    const float yLowerBottom = -(caida + alto);
    const float zLowerBottom = -0.18f;

    const float xLowerLeft   = -ancho;
    const float yLowerLeft   = -caida;
    const float zLowerLeft   = -0.42f;

    // ============================
    // CARA SUPERIOR
    // ============================

    glBegin(GL_QUADS);
    glColor3f(0.72f, 0.95f, 0.08f);
    glVertex3f(xTop,    yTop,    zTop);
    glVertex3f(xRight,  yRight,  zRight);
    glVertex3f(xBottom, yBottom, zBottom);
    glVertex3f(xLeft,   yLeft,   zLeft);
    glEnd();

    // ============================
    // CARA IZQUIERDA
    // ============================

    glBegin(GL_QUADS);
    glColor3f(0.52f, 0.78f, 0.08f);
    glVertex3f(xLeft,        yLeft,        zLeft);
    glVertex3f(xBottom,      yBottom,      zBottom);

    glColor3f(0.28f, 0.52f, 0.10f);
    glVertex3f(xLowerBottom, yLowerBottom, zLowerBottom);
    glVertex3f(xLowerLeft,   yLowerLeft,   zLowerLeft);
    glEnd();

    // ============================
    // CARA DERECHA
    // ============================

    glBegin(GL_QUADS);
    glColor3f(0.60f, 0.84f, 0.10f);
    glVertex3f(xBottom,      yBottom,      zBottom);
    glVertex3f(xRight,       yRight,       zRight);

    glColor3f(0.32f, 0.58f, 0.12f);
    glVertex3f(xLowerRight,  yLowerRight,  zLowerRight);
    glVertex3f(xLowerBottom, yLowerBottom, zLowerBottom);
    glEnd();

    glEnable(GL_LIGHTING);
}

void Renderer::dibujarVacas(const Modelo& modelo, const Escena& escena) {
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);
    const GLfloat posicionLuz[] = { 2.0f, 3.0f, 4.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, posicionLuz);

    const GLsizei vertices = modelo.triangulos * 3;
    for (const Instancia& vaca : escena.vacas) {
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -DIST_CAM);
        glTranslatef(vaca.x, vaca.y, 0.0f);
        glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(vaca.giro, 0.0f, 1.0f, 0.0f);
        glScalef(vaca.escala, vaca.escala, vaca.escala);
        glColor3f(vaca.r, vaca.g, vaca.b);
        glDrawArrays(GL_TRIANGLES, 0, vertices);
    }
}

void Renderer::dibujarHUD(float fps, int vacas, int estrellas) {
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
    glVertex2f(240.0f, 88.0f);
    glVertex2f(8.0f, 88.0f);
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

    if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (culling_) glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::dibujar(const Modelo& modelo, const Escena& escena,
                        const CampoEstrellas& campo, float fps) {
    glClearColor(0.008f, 0.012f, 0.045f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    dibujarEstrellas(campo);
    dibujarPlanetas(escena);
    dibujarPlataforma(escena);
    dibujarVacas(modelo, escena);
    dibujarHUD(fps, static_cast<int>(escena.vacas.size()),
               static_cast<int>(campo.estrellas.size()));
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

    constexpr int SEGMENTOS = 32;
    constexpr int LATITUDES = 16;
    constexpr int SEGMENTOS_ARO = 64;

    for (const Planeta& p : escena.planetas) {
        glPushMatrix();
        glTranslatef(p.x, p.y, -2.0f);
        glScalef(p.escala, p.escala, p.escala);

        const int textura = std::clamp(p.textura, 0, 3);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturasPlanetas_[textura]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.98f);

        // La esfera gira sobre su eje Y; al rotar la geometria también rota
        // el mapa UV y el movimiento de la textura se vuelve visible.
        glPushMatrix();
        glRotatef(p.giro, 0.0f, 1.0f, 0.0f);
        glBegin(GL_QUADS);
        for (int i = 0; i < SEGMENTOS; ++i) {
            const float u1 = static_cast<float>(i) /
                             static_cast<float>(SEGMENTOS);
            const float u2 = static_cast<float>(i + 1) /
                             static_cast<float>(SEGMENTOS);
            const float theta1 = 2.0f * PI * u1;
            const float theta2 = 2.0f * PI * u2;

            for (int j = 0; j < LATITUDES; ++j) {
                const float v1 = static_cast<float>(j) /
                                 static_cast<float>(LATITUDES);
                const float v2 = static_cast<float>(j + 1) /
                                 static_cast<float>(LATITUDES);
                const float phi1 = PI * v1;
                const float phi2 = PI * v2;
                const float radio1 = std::sin(phi1);
                const float radio2 = std::sin(phi2);

                glTexCoord2f(u1, v1);
                glVertex3f(radio1 * std::cos(theta1), std::cos(phi1),
                           radio1 * std::sin(theta1));
                glTexCoord2f(u2, v1);
                glVertex3f(radio1 * std::cos(theta2), std::cos(phi1),
                           radio1 * std::sin(theta2));
                glTexCoord2f(u2, v2);
                glVertex3f(radio2 * std::cos(theta2), std::cos(phi2),
                           radio2 * std::sin(theta2));
                glTexCoord2f(u1, v2);
                glVertex3f(radio2 * std::cos(theta1), std::cos(phi2),
                           radio2 * std::sin(theta1));
            }
        }
        glEnd();
        glPopMatrix();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        if (p.tieneAro) {
            // Solo dos planetas poseen aro. La prueba de profundidad oculta
            // su mitad posterior cuando pasa detras de la esfera.
            glRotatef(p.rotacionAro, 0.0f, 0.0f, 1.0f);
            glRotatef(p.inclinacionAro, 1.0f, 0.0f, 0.0f);
            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i <= SEGMENTOS_ARO; ++i) {
                const float angulo = 2.0f * PI *
                    static_cast<float>(i) /
                    static_cast<float>(SEGMENTOS_ARO);
                const float coseno = std::cos(angulo);
                const float seno = std::sin(angulo);
                const float pulso =
                    0.72f + 0.28f * std::sin(angulo * 5.0f);

                glColor4f(p.r * pulso, p.g * pulso, p.b * pulso, 0.18f);
                glVertex3f(1.18f * coseno, 1.18f * seno, 0.0f);
                glColor4f(std::min(1.0f, p.r * 1.45f),
                          std::min(1.0f, p.g * 1.45f),
                          std::min(1.0f, p.b * 1.45f), 0.62f);
                glVertex3f(1.72f * coseno, 1.72f * seno, 0.0f);
            }
            glEnd();
        }

        glPopMatrix();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    if (culling_) glEnable(GL_CULL_FACE);
    if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
