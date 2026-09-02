#ifndef ESCENA_H
#define ESCENA_H

#include <vector>

// Estado de cada vaca. No depende de SDL ni OpenGL: el render solo lee esto.
// actualizarEscena combina un lazo O(N^2) (separacion, compute-bound) con
// otro O(N) (integracion/rebote, memory-bound); el contraste es intencional.

struct Instancia {
    float x = 0.0f, y = 0.0f;   // centro, en el plano z = 0
    float giro = 0.0f;          // rotacion actual sobre el eje Y, en grados
    float velGiro = 0.0f;       // grados por segundo
    float escala = 1.0f;        // el modelo llega normalizado a tamano 2
    float r = 1.0f, g = 1.0f, b = 1.0f;

    // Velocidad lineal de la vaca sobre la superficie del semicirculo.
    float vx = 0.0f;
    float vy = 0.0f;
};

struct Planeta {
    float x = 0.0f;
    float y = 0.0f;
    float escala = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    int textura = 0;
    float vx = 0.0f;
    float giro = 0.0f;
    float velGiro = 0.0f;
    bool tieneAro = false;
    float inclinacionAro = 60.0f;

    // Orientacion fija del aro (no se anima, a diferencia de 'giro').
    float anguloAroInicial = 0.0f;
};

// El OVNI sobrevuela la plataforma en una trayectoria de Lissajous. Es
// decorativo: su posicion no afecta el movimiento de las vacas. O(1) por
// frame, no compite por tiempo con la parte O(N^2).
struct Ovni {
    float x = 0.0f;
    float y = 0.0f;
    // Desplazamiento hacia la camara (se ve en frente y mas grande) sin
    // cambiar su posicion 2D real en la simulacion.
    float z = 0.9f;
    float escala = 0.28f;  // chico: sobrevuela la manada, no la tapa
    float giro = 0.0f;            // rotacion visual sobre el eje Y
    float velGiro = -50.0f;       // grados por segundo
    float fase = 0.0f;            // acumulador de tiempo para la trayectoria
};

struct Escena {
    std::vector<Instancia> vacas;

    // Mitad del area visible en el plano z = 0. Se calcula desde la camara
    // y sirve para repartir las instancias y, mas adelante, para rebotar.
    float mitadAncho = 1.0f;
    float mitadAlto  = 1.0f;

    // Semicirculo compartido por simulacion y render: el diametro se apoya
    // en el borde inferior de pantalla, como el horizonte de un planeta.
    // circuloCentroY queda fuera de pantalla; las vacas nunca bajan de ahi.
    float circuloRadio = 1.0f;
    float circuloCentroY = 0.0f;

    std::vector<Planeta> planetas;
    Ovni ovni;

    // radioSeparacion: que tan cerca deben estar dos vacas para repelerse.
    // 0.8 reparte bien la manada sin amontonarla y ya se nota desde ~3000 vacas.
    float radioSeparacion = 0.8f;   // distancia a la que dos vacas se repelen
    float fuerzaSeparacion = 2.0f;  // aceleracion maxima de la separacion
    float rapidezMax = 2.6f;        // limite de rapidez tras aplicar fuerzas

    // Jaloncito hacia cohesionCentroY que evita que la separacion amontone
    // a las vacas contra el borde. O(1) por vaca, se aplica en la pasada B.
    float fuerzaCohesion = 1.0f;

    // Punto hacia el que jala la cohesion (ver arriba). Se fija en
    // crearEscena a una altura comoda dentro de la cupula visible.
    float cohesionCentroY = 0.0f;

    // Aceleracion acumulada por vaca en la pasada O(N^2). Vive en la Escena
    // (no es local a la funcion) para no reservar memoria cada frame; su
    // tamano se fija en crearEscena y no cambia mientras corre el programa.
    std::vector<float> ax;
    std::vector<float> ay;

    // Distancia de la vaca mas cercana al OVNI en el ultimo frame; se
    // calcula con una reduccion dentro de la misma pasada O(N^2).
    float distanciaMinimaOvni = 0.0f;
};

// Reparte 'n' instancias dentro del semicirculo. La escala se deriva de una
// grilla virtual para que continue dependiendo de la cantidad de vacas.
void crearEscena(Escena& e, int n,
                 float mitadAncho, float mitadAlto,
                 unsigned semilla = 1234u);

// Avanza vacas (separacion O(N^2) + cohesion/rebote O(N)), el OVNI y los
// planetas. msPasadaA/msPasadaB devuelven el costo de cada pasada por
// separado, en milisegundos (util para comparar schedules).
void actualizarEscena(Escena& e, float dt, bool usarOpenMP,
                      double* msPasadaA = nullptr,
                      double* msPasadaB = nullptr);

#endif // ESCENA_H
