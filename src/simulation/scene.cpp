#include "zipzip/simulation/scene.h"

#include "zipzip/core/camara.h"
#include "zipzip/core/rng.h"

#include <chrono>
#include <cmath>
#include <cstdint>

namespace {

using zipzip::PI;
using zipzip::Rng;

constexpr float MARGEN_INICIAL_CIRCULO = 0.85f;
constexpr float MARGEN_REBOTE_CIRCULO = 0.995f;

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

    // El circulo nace del borde inferior de la pantalla: su centro real
    // (circuloCentroY) queda fuera de vista, en y = -mitadAlto. Solo la
    // mitad de arriba (el semicirculo) esta en juego.
    e.circuloCentroY = -mitadAlto;
    e.circuloRadio = mitadAncho * 0.95f;

    // Punto de cohesion: un poco por debajo de la mitad de la cupula, para
    // que la manada se reparta por toda la superficie visible sin apilarse
    // contra el borde plano de abajo.
    e.cohesionCentroY = e.circuloCentroY + e.circuloRadio * 0.55f;

    e.vacas.clear();
    e.vacas.reserve(static_cast<size_t>(n));

    // La grilla solo determina una escala dependiente de N. Las posiciones se
    // generan aparte y de manera uniforme dentro del semicirculo.
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
        // radio = R*sqrt(u) da distribucion uniforme dentro de un disco;
        // el angulo en [0, pi] deja solo la mitad de arriba (el semicirculo).
        const float radioMuestra = e.circuloRadio * MARGEN_INICIAL_CIRCULO *
            std::sqrt(rng.entre(0.0f, 1.0f));
        const float anguloMuestra = rng.entre(0.0f, PI);
        ins.x = radioMuestra * std::cos(anguloMuestra);
        ins.y = e.circuloCentroY + radioMuestra * std::sin(anguloMuestra);

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

    // Buffers de la pasada O(N^2): se reservan una sola vez aqui, no en cada
    // frame de actualizarEscena.
    e.ax.assign(e.vacas.size(), 0.0f);
    e.ay.assign(e.vacas.size(), 0.0f);

    e.ovni = Ovni();
    e.ovni.x = 0.0f;
    e.ovni.y = e.cohesionCentroY;

    constexpr int NUM_PLANETAS = 4;
    e.planetas.clear();
    e.planetas.reserve(NUM_PLANETAS);

    struct ConfiguracionPlaneta {
        float factorX;
        float factorY;
        float escala;
        float r;
        float g;
        float b;
        float velocidadX;
        float giroInicial;
        float velocidadGiro;
        bool tieneAro;
        float inclinacionAro;
        float anguloAroInicial;
    };

    // A la profundidad usada por el renderer el area visible es mayor que en
    // z = 0. Estos factores colocan los planetas cerca de las cuatro esquinas
    // sin recortarlos ni invadir la plataforma central.
    constexpr ConfiguracionPlaneta configuraciones[NUM_PLANETAS] = {
        {-1.12f,  1.22f, 0.40f, 0.55f, 0.34f, 0.78f,
          0.28f,  15.0f,  9.0f, true,  64.0f, -18.0f},
        { 1.12f,  0.92f, 0.35f, 0.28f, 0.62f, 0.88f,
         -0.36f, 130.0f, 14.0f, false, 58.0f,  24.0f},
        {-1.12f,  0.05f, 0.34f, 0.82f, 0.42f, 0.20f,
          0.44f, 245.0f, 20.0f, true,  67.0f,  16.0f},
        { 1.12f,  0.38f, 0.38f, 0.34f, 0.72f, 0.48f,
         -0.31f, 310.0f, 11.0f, false, 61.0f, -26.0f},
    };

    for (int i = 0; i < NUM_PLANETAS; ++i) {
        const ConfiguracionPlaneta& config = configuraciones[i];
        Planeta planeta;
        planeta.x = mitadAncho * config.factorX;
        planeta.y = mitadAlto * config.factorY;
        planeta.escala = config.escala;
        planeta.r = config.r;
        planeta.g = config.g;
        planeta.b = config.b;
        planeta.textura = i;
        planeta.vx = config.velocidadX;
        planeta.giro = config.giroInicial;
        planeta.velGiro = config.velocidadGiro;
        planeta.tieneAro = config.tieneAro;
        planeta.inclinacionAro = config.inclinacionAro;
        planeta.anguloAroInicial = config.anguloAroInicial;
        e.planetas.push_back(planeta);
    }
}

void actualizarEscena(Escena& e, float dt, bool usarOpenMP,
                      double* msPasadaA, double* msPasadaB) {
    const long cantidadVacas = static_cast<long>(e.vacas.size());

    // El OVNI recorre una trayectoria de Lissajous sobre la cupula. Es O(1)
    // por frame: no compite por tiempo con la parte que se mide.
    Ovni& ovni = e.ovni;
    ovni.fase += dt;
    ovni.giro += ovni.velGiro * dt;
    if (ovni.giro >= 360.0f) ovni.giro -= 360.0f;
    if (ovni.giro < 0.0f)    ovni.giro += 360.0f;

    constexpr float FRECUENCIA_X = 0.55f;
    constexpr float FRECUENCIA_Y = 0.35f;
    constexpr float DESFASE_Y = 1.3f;
    // Franja vertical comoda dentro de la cupula (no llega al borde plano de
    // abajo ni se acerca demasiado a la cima).
    const float amplitudX = e.circuloRadio * 0.75f;
    const float amplitudY = e.circuloRadio * 0.28f;
    ovni.x = amplitudX * std::sin(ovni.fase * FRECUENCIA_X);
    ovni.y = e.cohesionCentroY +
             amplitudY * std::sin(ovni.fase * FRECUENCIA_Y + DESFASE_Y);

    // Pasada A: cada vaca compara contra todas las demas (O(N^2), compute-bound).
    // Solo lee el arreglo de vacas y escribe su propio ax[i]/ay[i]: sin
    // dependencias entre iteraciones, el resultado es el mismo con 1 o N hilos.
    const float radioSeparacionAlCuadrado =
        e.radioSeparacion * e.radioSeparacion;

    // Minimo global de distancia al OVNI, solo para el HUD (no aplica
    // fuerza): demuestra una reduccion dentro de esta misma pasada.
    float distanciaMinimaOvni = 1e9f;

    const auto inicioA = std::chrono::steady_clock::now();
#ifdef _OPENMP
#pragma omp parallel for if(usarOpenMP) schedule(runtime) reduction(min:distanciaMinimaOvni)
#else
    (void)usarOpenMP;
#endif
    for (long indiceVaca = 0; indiceVaca < cantidadVacas; ++indiceVaca) {
        const Instancia& vacaActual = e.vacas[static_cast<size_t>(indiceVaca)];
        float aceleracionX = 0.0f;
        float aceleracionY = 0.0f;

        for (long indiceVecina = 0; indiceVecina < cantidadVacas;
             ++indiceVecina) {
            if (indiceVecina == indiceVaca) continue;
            const Instancia& vacaVecina =
                e.vacas[static_cast<size_t>(indiceVecina)];

            // Vector de la vecina hacia la vaca actual: si estan mas cerca
            // que radioSeparacion, empuja a vacaActual en esta direccion
            // (alejandola de vacaVecina).
            const float diferenciaX = vacaActual.x - vacaVecina.x;
            const float diferenciaY = vacaActual.y - vacaVecina.y;
            const float distanciaAlCuadrado =
                diferenciaX * diferenciaX + diferenciaY * diferenciaY;
            if (distanciaAlCuadrado < radioSeparacionAlCuadrado &&
                distanciaAlCuadrado > 1e-8f) {
                const float distancia = std::sqrt(distanciaAlCuadrado);
                const float fuerza = e.fuerzaSeparacion *
                    (e.radioSeparacion - distancia) / e.radioSeparacion;
                aceleracionX += (diferenciaX / distancia) * fuerza;
                aceleracionY += (diferenciaY / distancia) * fuerza;
            }
        }

        // Distancia al OVNI: solo alimenta el HUD, no aplica fuerza.
        const float diferenciaXOvni = vacaActual.x - ovni.x;
        const float diferenciaYOvni = vacaActual.y - ovni.y;
        const float distanciaAlOvni = std::sqrt(
            diferenciaXOvni * diferenciaXOvni + diferenciaYOvni * diferenciaYOvni);
        if (distanciaAlOvni < distanciaMinimaOvni) {
            distanciaMinimaOvni = distanciaAlOvni;
        }

        e.ax[static_cast<size_t>(indiceVaca)] = aceleracionX;
        e.ay[static_cast<size_t>(indiceVaca)] = aceleracionY;
    }

    e.distanciaMinimaOvni = distanciaMinimaOvni;
    if (msPasadaA) {
        *msPasadaA = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - inicioA).count();
    }

    // Pasada B: cada vaca solo lee/escribe su propio estado (O(N),
    // memory-bound). Aqui vive el false sharing de schedule(static,1):
    // Instancia mide 40 bytes, dos vacas consecutivas caen en la misma linea.
    const auto inicioB = std::chrono::steady_clock::now();
#ifdef _OPENMP
#pragma omp parallel for if(usarOpenMP) schedule(runtime)
#endif
    for (long indiceVaca = 0; indiceVaca < cantidadVacas; ++indiceVaca) {
        Instancia& vaca = e.vacas[static_cast<size_t>(indiceVaca)];

        // Cohesion: jaloncito hacia cohesionCentroY, proporcional a la
        // distancia. Es la unica fuerza no repulsiva; sin ella la separacion
        // amontonaria a la manada contra el borde.
        const float aceleracionCohesionX =
            -e.fuerzaCohesion * (vaca.x / e.circuloRadio);
        const float aceleracionCohesionY = -e.fuerzaCohesion *
            ((vaca.y - e.cohesionCentroY) / e.circuloRadio);

        vaca.vx += (e.ax[static_cast<size_t>(indiceVaca)] +
                    aceleracionCohesionX) * dt;
        vaca.vy += (e.ay[static_cast<size_t>(indiceVaca)] +
                    aceleracionCohesionY) * dt;

        const float rapidez = std::sqrt(vaca.vx * vaca.vx + vaca.vy * vaca.vy);
        if (rapidez > e.rapidezMax) {
            const float ajusteRapidez = e.rapidezMax / rapidez;
            vaca.vx *= ajusteRapidez;
            vaca.vy *= ajusteRapidez;
        }

        vaca.giro += vaca.velGiro * dt;
        if (vaca.giro >= 360.0f) vaca.giro -= 360.0f;
        if (vaca.giro < 0.0f)    vaca.giro += 360.0f;

        vaca.x += vaca.vx * dt;
        vaca.y += vaca.vy * dt;

        // Borde curvo: normal = direccion radial desde circuloCentroY.
        // Formula de reflexion v' = v - 2(v.n)n, ver docs/matematica_rebote_rombo.md.
        const float diferenciaCentroX = vaca.x;
        const float diferenciaCentroY = vaca.y - e.circuloCentroY;
        const float distanciaAlCentroAlCuadrado =
            diferenciaCentroX * diferenciaCentroX +
            diferenciaCentroY * diferenciaCentroY;

        if (distanciaAlCentroAlCuadrado >
            e.circuloRadio * e.circuloRadio) {
            const float distanciaAlCentro =
                std::sqrt(distanciaAlCentroAlCuadrado);
            const float normalX = diferenciaCentroX / distanciaAlCentro;
            const float normalY = diferenciaCentroY / distanciaAlCentro;

            const float productoPunto = vaca.vx * normalX + vaca.vy * normalY;

            // Solo se refleja si la vaca todavia se dirige hacia afuera. La
            // condicion contraria provocaba que se recolocara cada frame sin
            // cambiar su trayectoria, produciendo vibracion en los bordes.
            if (productoPunto > 0.0f) {
                vaca.vx -= 2.0f * productoPunto * normalX;
                vaca.vy -= 2.0f * productoPunto * normalY;
            }

            // Proyeccion radial a un punto apenas interior. El margen evita
            // detectar de nuevo el mismo choque por error de punto flotante.
            const float ajustePosicion =
                (e.circuloRadio * MARGEN_REBOTE_CIRCULO) / distanciaAlCentro;
            vaca.x = diferenciaCentroX * ajustePosicion;
            vaca.y = e.circuloCentroY + diferenciaCentroY * ajustePosicion;
        }

        // Borde plano (diametro del circulo): rebote elastico simple.
        if (vaca.y < e.circuloCentroY) {
            if (vaca.vy < 0.0f) vaca.vy = -vaca.vy;
            vaca.y = e.circuloCentroY;
        }
    }

    if (msPasadaB) {
        *msPasadaB = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - inicioB).count();
    }

    // Los planetas recorren carriles horizontales detras de la plataforma.
    // El limite considera la perspectiva de z = -2 y el radio exterior del
    // aro para que cada planeta desaparezca por completo antes de reaparecer.
    for (Planeta& planeta : e.planetas) {
        planeta.x += planeta.vx * dt;
        planeta.giro += planeta.velGiro * dt;
        if (planeta.giro >= 360.0f) planeta.giro -= 360.0f;

        const float limite =
            e.mitadAncho * 1.50f + planeta.escala * 1.80f;
        if (planeta.x > limite) planeta.x = -limite;
        if (planeta.x < -limite) planeta.x = limite;
    }
}
