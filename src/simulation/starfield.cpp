#include "zipzip/simulation/starfield.h"

#include "zipzip/core/camara.h"
#include "zipzip/core/rng.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

using zipzip::PI;
using zipzip::Rng;

constexpr float FACTOR_FONDO = 1.25f;
constexpr int CANTIDAD_FUGACES = 3;

// EstrellaFugaz guarda su semilla como campo propio (no un Rng completo) para
// no arrastrar zipzip/core/rng.h hasta el header publico de la escena; aqui
// se usan directamente las funciones libres sobre ese campo.
void prepararFugaz(EstrellaFugaz& fugaz, const CampoEstrellas& campo,
                    bool primeraAparicion) {
    const bool desdeIzquierda =
        (zipzip::xorshift32(fugaz.estado) & 1u) == 0u;
    const float direccion = desdeIzquierda ? 1.0f : -1.0f;

    fugaz.x = direccion * -(campo.mitadAncho + 0.25f);
    fugaz.y = zipzip::uniformeEntre(fugaz.estado, campo.mitadAlto * 0.20f,
                                     campo.mitadAlto * 0.92f);
    fugaz.vx = direccion * zipzip::uniformeEntre(fugaz.estado, 3.8f, 5.2f);
    fugaz.vy = -zipzip::uniformeEntre(fugaz.estado, 1.5f, 2.5f);
    fugaz.longitud = zipzip::uniformeEntre(fugaz.estado, 0.55f, 0.95f);
    fugaz.duracion = zipzip::uniformeEntre(fugaz.estado, 1.35f, 2.10f);
    fugaz.edad = 0.0f;
    fugaz.brillo = 0.0f;
    fugaz.espera = primeraAparicion
        ? zipzip::uniformeEntre(fugaz.estado, 0.4f, 4.5f)
        : zipzip::uniformeEntre(fugaz.estado, 3.0f, 8.0f);
    fugaz.activa = false;
}

} // namespace

void crearCampoEstrellas(CampoEstrellas& campo, int cantidad,
                         float mitadAncho, float mitadAlto,
                         unsigned semilla) {
    if (cantidad < 1) cantidad = 1;

    // Las estrellas se dibujan un poco detras del plano z = 0. El factor
    // compensa la perspectiva para cubrir tambien los cuatro bordes del frame.
    campo.mitadAncho = mitadAncho * FACTOR_FONDO;
    campo.mitadAlto = mitadAlto * FACTOR_FONDO;
    campo.tiempo = 0.0f;
    campo.estrellas.clear();
    campo.estrellas.reserve(static_cast<size_t>(cantidad));

    Rng rng(semilla);
    const float aspecto = campo.mitadAncho / campo.mitadAlto;
    int columnas = static_cast<int>(
        std::ceil(std::sqrt(static_cast<float>(cantidad) * aspecto)));
    if (columnas < 1) columnas = 1;
    const int filas = (cantidad + columnas - 1) / columnas;
    const int celdas = columnas * filas;
    const float anchoCelda =
        (2.0f * campo.mitadAncho) / static_cast<float>(columnas);
    const float altoCelda =
        (2.0f * campo.mitadAlto) / static_cast<float>(filas);

    for (int indiceEstrella = 0; indiceEstrella < cantidad; ++indiceEstrella) {
        // Se recorren celdas espaciadas a lo largo de toda la grilla. El
        // jitter evita un patron rigido sin dejar zonas grandes vacias.
        const int celda = static_cast<int>(
            (static_cast<long long>(indiceEstrella) * celdas) / cantidad);
        const int columna = celda % columnas;
        const int fila = celda / columnas;

        Estrella estrella;
        estrella.x = -campo.mitadAncho +
            (static_cast<float>(columna) + rng.entre(0.08f, 0.92f)) * anchoCelda;
        estrella.y = -campo.mitadAlto +
            (static_cast<float>(fila) + rng.entre(0.08f, 0.92f)) * altoCelda;
        estrella.vx = rng.entre(-0.055f, 0.055f);
        estrella.vy = rng.entre(-0.025f, 0.025f);
        estrella.tamano = rng.entre(1.0f, 3.2f);
        estrella.brilloBase = rng.entre(0.35f, 0.72f);
        estrella.amplitud = rng.entre(0.18f, 0.42f);
        estrella.frecuencia = rng.entre(1.2f, 4.8f);
        estrella.fase = rng.entre(0.0f, 2.0f * PI);
        estrella.brillo = estrella.brilloBase;
        campo.estrellas.push_back(estrella);
    }

    campo.fugaces.clear();
    campo.fugaces.reserve(CANTIDAD_FUGACES);
    for (int indiceFugaz = 0; indiceFugaz < CANTIDAD_FUGACES; ++indiceFugaz) {
        EstrellaFugaz fugaz;
        fugaz.estado = semilla ^
            (0x9E3779B9u + static_cast<unsigned>(indiceFugaz) * 0x85EBCA6Bu);
        if (fugaz.estado == 0u) {
            fugaz.estado = static_cast<unsigned>(indiceFugaz + 1);
        }
        prepararFugaz(fugaz, campo, true);
        campo.fugaces.push_back(fugaz);
    }
}

void actualizarCampoEstrellas(CampoEstrellas& campo, float dt) {
    campo.tiempo += dt;

    // O(N) e independiente por estrella, igual que la integracion de las
    // vacas: se paraleliza por completitud, pero con pocas estrellas su
    // aporte al tiempo total es minimo frente a la interaccion O(N^2).
    const long cantidadEstrellas = static_cast<long>(campo.estrellas.size());
#ifdef _OPENMP
#pragma omp parallel for schedule(runtime)
#endif
    for (long i = 0; i < cantidadEstrellas; ++i) {
        Estrella& estrella = campo.estrellas[static_cast<size_t>(i)];
        estrella.x += estrella.vx * dt;
        estrella.y += estrella.vy * dt;

        if (estrella.x > campo.mitadAncho) estrella.x = -campo.mitadAncho;
        if (estrella.x < -campo.mitadAncho) estrella.x = campo.mitadAncho;
        if (estrella.y > campo.mitadAlto) estrella.y = -campo.mitadAlto;
        if (estrella.y < -campo.mitadAlto) estrella.y = campo.mitadAlto;

        const float pulso = 0.5f + 0.5f * std::sin(
            estrella.frecuencia * campo.tiempo + estrella.fase);
        estrella.brillo = std::clamp(
            estrella.brilloBase + estrella.amplitud * pulso, 0.0f, 1.0f);
    }

    for (EstrellaFugaz& fugaz : campo.fugaces) {
        if (!fugaz.activa) {
            fugaz.espera -= dt;
            if (fugaz.espera <= 0.0f) {
                fugaz.activa = true;
                fugaz.edad = 0.0f;
            }
            continue;
        }

        fugaz.x += fugaz.vx * dt;
        fugaz.y += fugaz.vy * dt;
        fugaz.edad += dt;

        const float progreso = std::clamp(
            fugaz.edad / fugaz.duracion, 0.0f, 1.0f);
        fugaz.brillo = std::sin(PI * progreso);

        const bool fuera =
            fugaz.x < -campo.mitadAncho - fugaz.longitud ||
            fugaz.x >  campo.mitadAncho + fugaz.longitud ||
            fugaz.y < -campo.mitadAlto - fugaz.longitud;

        if (fugaz.edad >= fugaz.duracion || fuera) {
            prepararFugaz(fugaz, campo, false);
        }
    }
}
