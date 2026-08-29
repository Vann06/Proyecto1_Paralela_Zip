#include "zipzip/rendering/renderer.h"

#include <SDL2/SDL_opengl.h>

#include <cmath>
#include <cstdio>

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

    configurarProyeccion();
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

    glPointSize(1.0f);
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
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -DIST_CAM);

    for (const Planeta& p : escena.planetas) {
        glPushMatrix();
        //fondo
        glTranslatef(p.x, p.y, -2.0f);
        glScalef(p.escala, p.escala, p.escala);
        
        glColor4f(p.r, p.g, p.b, 0.6f);
        
        const int segmentos = 12;
        glBegin(GL_QUADS);
        for (int i = 0; i < segmentos; ++i) {
            float theta1 = (2.0f * PI * i) / segmentos;
            float theta2 = (2.0f * PI * (i + 1)) / segmentos;
            
            for (int j = 0; j < segmentos / 2; ++j) {
                float phi1 = (PI * j) / (segmentos / 2);
                float phi2 = (PI * (j + 1)) / (segmentos / 2);
                
                glVertex3f(sin(phi1) * cos(theta1), sin(phi1) * sin(theta1), cos(phi1));
                glVertex3f(sin(phi1) * cos(theta2), sin(phi1) * sin(theta2), cos(phi1));
                glVertex3f(sin(phi2) * cos(theta2), sin(phi2) * sin(theta2), cos(phi2));
                glVertex3f(sin(phi2) * cos(theta1), sin(phi2) * sin(theta1), cos(phi2));
            }
        }
        glEnd();
        
        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    if (culling_) glEnable(GL_CULL_FACE);
}