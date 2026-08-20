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

struct CampoEstrellas {
    std::vector<Estrella> estrellas;
    float mitadAncho = 1.0f;
    float mitadAlto = 1.0f;
    float tiempo = 0.0f;
};

// Crea un fondo determinista. La misma semilla produce la misma distribucion.
void crearCampoEstrellas(CampoEstrellas& campo, int cantidad,
                         float mitadAncho, float mitadAlto,
                         unsigned semilla = 4321u);

// Mueve las estrellas lentamente, actualiza su brillo y las envuelve en bordes.
void actualizarCampoEstrellas(CampoEstrellas& campo, float dt);

#endif // ZIPZIP_SIMULATION_STARFIELD_H
