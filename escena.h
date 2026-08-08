#ifndef ESCENA_H
#define ESCENA_H

#include <vector>

// Conjunto de instancias del mismo modelo repartidas en la ventana.
//
// Igual que el loader, este modulo no depende de SDL ni de OpenGL: solo
// mantiene el estado de cada vaca. El render lee ese estado y lo dibuja.
// Cuando toque paralelizar, el lazo caro es 'actualizarEscena': cada
// instancia se mueve sin mirar a las demas.

struct Instancia {
    float x = 0.0f, y = 0.0f;   // centro, en el plano z = 0
    float giro = 0.0f;          // rotacion actual sobre el eje Y, en grados
    float velGiro = 0.0f;       // grados por segundo
    float escala = 1.0f;        // el modelo llega normalizado a tamano 2
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct Escena {
    std::vector<Instancia> vacas;

    // Mitad del area visible en el plano z = 0. Se calcula desde la camara
    // y sirve para repartir las instancias y, mas adelante, para rebotar.
    float mitadAncho = 1.0f;
    float mitadAlto  = 1.0f;
};

// Reparte 'n' instancias sobre el area visible usando una grilla con jitter:
// quedan bien distribuidas y con tamano suficiente para verse todas.
void crearEscena(Escena& e, int n,
                 float mitadAncho, float mitadAlto,
                 unsigned semilla = 1234u);

// Avanza la escena 'dt' segundos. Por ahora solo gira cada instancia.
void actualizarEscena(Escena& e, float dt);

#endif // ESCENA_H
