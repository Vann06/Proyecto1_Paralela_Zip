#include "zipzip/simulation/scene.h"

#include <cmath>
#include <cstdint>

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr float MARGEN_INICIAL_ROMBO = 0.78f;
constexpr float MARGEN_REBOTE_ROMBO = 0.995f;

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
        return a + (b - a) * (static_cast<float>(siguiente()) / 4294967296.0f);
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

// En los vertices del rombo una coordenada puede ser exactamente cero. En
// ese caso la componente de la velocidad permite escoger uno de los dos lados
// adyacentes sin dejar la vaca oscilando entre ambas normales.
float signoLado(float coordenada, float velocidad) {
    if (coordenada > 0.0f) return 1.0f;
    if (coordenada < 0.0f) return -1.0f;
    return velocidad >= 0.0f ? 1.0f : -1.0f;
}

} // namespace anonimo

void crearEscena(Escena& e, int n,
                 float mitadAncho, float mitadAlto,
                 unsigned semilla) {
    if (n < 1) n = 1;

    e.mitadAncho = mitadAncho;
    e.mitadAlto  = mitadAlto;
    e.romboAncho = mitadAncho * 0.85f;
    e.romboAlto = mitadAlto * 0.60f;
    e.vacas.clear();
    e.vacas.reserve(static_cast<size_t>(n));

    // La grilla solo determina una escala dependiente de N. Las posiciones se
    // generan aparte y de manera uniforme dentro del rombo.
    float aspecto = mitadAncho / mitadAlto;
    int cols = static_cast<int>(
        std::ceil(std::sqrt(static_cast<float>(n) * aspecto)));
    if (cols < 1) cols = 1;
    int filas = (n + cols - 1) / cols;

    float anchoCelda = (2.0f * mitadAncho) / static_cast<float>(cols);
    float altoCelda  = (2.0f * mitadAlto)  / static_cast<float>(filas);
    float celdaMin   = std::fmin(anchoCelda, altoCelda);

    // El modelo llega con su dimension mayor igual a 2. Se usa un factor un
    // poco menor que el original, pero la escala sigue disminuyendo con N.
    float escalaBase = celdaMin * 0.28f;

    Rng rng(semilla);

    for (int i = 0; i < n; ++i) {
        Instancia ins;
        // La transformacion (u + v - 1, u - v), con u y v uniformes en
        // [0, 1), distribuye puntos uniformemente dentro del rombo unidad.
        // El margen evita que varias vacas nazcan pegadas a los cuatro lados.
        const float u = rng.entre(0.0f, 1.0f);
        const float v = rng.entre(0.0f, 1.0f);
        ins.x = (u + v - 1.0f) * e.romboAncho * MARGEN_INICIAL_ROMBO;
        ins.y = (u - v) * e.romboAlto * MARGEN_INICIAL_ROMBO;

        ins.giro    = rng.entre(0.0f, 360.0f);
        ins.velGiro = rng.entre(20.0f, 70.0f) * (rng.entre(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f);
        ins.escala  = escalaBase * rng.entre(0.85f, 1.0f);

        // Velocidad lineal con direccion inicial libre.
        float rapidez = rng.entre(0.5f, 1.5f);
        float angulo = rng.entre(0.0f, 2.0f * PI);
        ins.vx = rapidez * std::cos(angulo);
        ins.vy = rapidez * std::sin(angulo);

        desdeTono(rng.entre(0.0f, 1.0f), ins.r, ins.g, ins.b);

        e.vacas.push_back(ins);
    }

    //constexpr float PI = 3.14159265358979323846f;
    const int NUM_PLANETAS = 5;

    e.planetas.clear();
    e.planetas.reserve(NUM_PLANETAS);

    struct ColorPlaneta {
        float r, g, b;
    };
    
    const ColorPlaneta coloresPlanetas[] = {
        {0.35f, 0.25f, 0.45f},  //morado
        {0.45f, 0.30f, 0.20f},  //cafe
        {0.20f, 0.40f, 0.45f},  //azul verdoso
        {0.50f, 0.35f, 0.15f},  //naranja
        {0.15f, 0.35f, 0.30f}   //verde
    };

    Rng rngPlanetas(semilla + 999);

    for (int i = 0; i < NUM_PLANETAS; ++i) {
        Planeta p;
        //pos. planetas
        p.x = rngPlanetas.entre(-mitadAncho * 0.85f, mitadAncho * 0.85f);
        p.y = rngPlanetas.entre(-mitadAlto * 0.85f, mitadAlto * 0.85f);
        
        //tamaños
        p.escala = rngPlanetas.entre(0.20f, 0.50f);
        
        int idx = i % 5;
        p.r = coloresPlanetas[idx].r;
        p.g = coloresPlanetas[idx].g;
        p.b = coloresPlanetas[idx].b;
        
        e.planetas.push_back(p);
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

        v.x += v.vx * dt;
        v.y += v.vy * dt;

        const float a = e.romboAncho;
        const float b = e.romboAlto;

        // Ecuacion del rombo: |x| / a + |y| / b <= 1.
        const float distanciaRombo =
            std::fabs(v.x) / a +
            std::fabs(v.y) / b;

        if (distanciaRombo > 1.0f) {
            float nx = signoLado(v.x, v.vx) / a;
            float ny = signoLado(v.y, v.vy) / b;

            // Normal unitaria hacia afuera del lado alcanzado.
            const float longitud = std::sqrt(nx * nx + ny * ny);
            nx /= longitud;
            ny /= longitud;

            const float producto = v.vx * nx + v.vy * ny;

            // Solo se refleja si la vaca todavia se dirige hacia afuera. La
            // condicion contraria provocaba que se recolocara cada frame sin
            // cambiar su trayectoria, produciendo vibracion en los bordes.
            if (producto > 0.0f) {
                v.vx -= 2.0f * producto * nx;
                v.vy -= 2.0f * producto * ny;
            }

            // Proyeccion radial a un punto apenas interior. El margen evita
            // detectar de nuevo el mismo choque por error de punto flotante.
            const float ajuste = MARGEN_REBOTE_ROMBO / distanciaRombo;
            v.x *= ajuste;
            v.y *= ajuste;
        }
    }
}
