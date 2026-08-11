#include "escena.h"

#include <cmath>
#include <cstdint>

namespace {

// Generador propio en lugar de <random>: es determinista entre maquinas y
// compiladores, asi la misma semilla da siempre la misma escena. Eso importa
// para comparar tiempos entre la version serial y la paralela.
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t semilla) : s(semilla ? semilla : 1u) {}

    uint32_t siguiente() {              // xorshift32
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    // Real uniforme en [a, b).
    float entre(float a, float b) {
        return a + (b - a) * (siguiente() / 4294967296.0f);
    }
};

// Color desde un tono en [0,1), con saturacion y valor fijos.
// Evita traer una libreria de color solo para esto.
void desdeTono(float h, float& r, float& g, float& b) {
    const float s = 0.45f, v = 1.0f;
    float sector = h * 6.0f;
    int   i = static_cast<int>(sector) % 6;
    float f = sector - std::floor(sector);
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

} // namespace anonimo

void crearEscena(Escena& e, int n,
                 float mitadAncho, float mitadAlto,
                 unsigned semilla) {
    if (n < 1) n = 1;

    e.mitadAncho = mitadAncho;
    e.mitadAlto  = mitadAlto;
    e.vacas.clear();
    e.vacas.reserve(static_cast<size_t>(n));

    // Grilla con la misma proporcion que el area visible, para que las celdas
    // salgan lo mas cuadradas posible y no se desperdicie espacio.
    float aspecto = mitadAncho / mitadAlto;
    int cols = static_cast<int>(std::ceil(std::sqrt(n * aspecto)));
    if (cols < 1) cols = 1;
    int filas = (n + cols - 1) / cols;

    float anchoCelda = (2.0f * mitadAncho) / cols;
    float altoCelda  = (2.0f * mitadAlto)  / filas;
    float celdaMin   = std::fmin(anchoCelda, altoCelda);

    // El modelo llega con su dimension mayor igual a 2. Al girar sobre Y su
    // huella crece hasta la diagonal, por eso se deja holgura: 0.33 en vez
    // del 0.5 que llenaria la celda por completo.
    float escalaBase = celdaMin * 0.33f;

    Rng rng(semilla);

    for (int i = 0; i < n; ++i) {
        int col = i % cols;
        int fila = i / cols;

        Instancia ins;
        // Centro de la celda, mas un desplazamiento chico para que no se vea
        // como una tabla perfecta.
        ins.x = -mitadAncho + (col  + 0.5f) * anchoCelda + rng.entre(-0.18f, 0.18f) * anchoCelda;
        ins.y = -mitadAlto  + (fila + 0.5f) * altoCelda  + rng.entre(-0.18f, 0.18f) * altoCelda;

        ins.giro    = rng.entre(0.0f, 360.0f);
        ins.velGiro = rng.entre(20.0f, 70.0f) * (rng.entre(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f);
        ins.escala  = escalaBase * rng.entre(0.85f, 1.0f);

        //vaca
        float rapidez = rng.entre(0.5f, 1.5f);
        float angulo = rng.entre(0.0f, 2.0f * 3.14159f);
        ins.vx = rapidez* std::cos(angulo);
        ins.vy = rapidez * std::sin(angulo);


        desdeTono(rng.entre(0.0f, 1.0f), ins.r, ins.g, ins.b);

        e.vacas.push_back(ins);
    }
}

void actualizarEscena(Escena& e, float dt) {
    // Lazo independiente por instancia: este es el candidato natural para
    // '#pragma omp parallel for' en la version paralela.
    for (size_t i = 0; i < e.vacas.size(); ++i) {
        Instancia& v = e.vacas[i];

        v.giro += v.velGiro * dt;
        if (v.giro >= 360.0f) v.giro -= 360.0f;
        if (v.giro < 0.0f)    v.giro += 360.0f;

        //rebote bordes vacas
        v.x += v.vx*dt;
        v.y += v.vy*dt;

        if (v.x > e.mitadAncho){
            v.x = e.mitadAncho;
            v.vx = -v.vx;
        } else if (v.x < -e.mitadAncho) {
            v.x = -e.mitadAncho;
            v.vx = -v.vx;
        }

        if(v.y > e.mitadAlto){
            v.y = e.mitadAlto;
            v.vy = -v.vy;
        }else if (v.y < -e.mitadAlto){
            v.y = -e.mitadAlto;
            v.vy = -v.vy;
        }
    }
}
