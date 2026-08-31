#ifndef ZIPZIP_SIMULATION_STARFIELD_H
#define ZIPZIP_SIMULATION_STARFIELD_H

#include <vector>

struct Estrella {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float tamano = 1.0f;
    float brilloBase = 0.6f;
    float amplitud = 0.3f;
    float frecuencia = 1.0f;
    float fase = 0.0f;
    float brillo = 1.0f;
};

struct EstrellaFugaz {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float longitud = 0.6f;
    float brillo = 1.0f;
    float edad = 0.0f;
    float duracion = 1.0f;
    float espera = 0.0f;
    unsigned estado = 1u;
    bool activa = false;
};

struct CampoEstrellas {
    std::vector<Estrella> estrellas;
    std::vector<EstrellaFugaz> fugaces;
    float mitadAncho = 1.0f;
    float mitadAlto = 1.0f;
    float tiempo = 0.0f;
};

// Crea un fondo determinista. La misma semilla produce la misma distribucion.
void crearCampoEstrellas(CampoEstrellas& campo, int cantidad,
                         float mitadAncho, float mitadAlto,
                         unsigned semilla = 4321u);

// Mueve las estrellas, actualiza su brillo y anima las estrellas fugaces.
void actualizarCampoEstrellas(CampoEstrellas& campo, float dt,
                              bool usarOpenMP = false);

#endif // ZIPZIP_SIMULATION_STARFIELD_H
