#include "zipzip/simulation/starfield.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

constexpr float PI = 3.14159265358979323846f;

struct Rng {
    uint32_t estado;

    explicit Rng(uint32_t semilla) : estado(semilla ? semilla : 1u) {}

    uint32_t siguiente() {
        estado ^= estado << 13;
        estado ^= estado >> 17;
        estado ^= estado << 5;
        return estado;
    }

    float entre(float minimo, float maximo) {
        return minimo + (maximo - minimo) *
               (siguiente() / 4294967296.0f);
    }
};

} // namespace

void crearCampoEstrellas(CampoEstrellas& campo, int cantidad,
                         float mitadAncho, float mitadAlto,
                         unsigned semilla) {
    if (cantidad < 1) cantidad = 1;

    campo.mitadAncho = mitadAncho;
    campo.mitadAlto = mitadAlto;
    campo.tiempo = 0.0f;
    campo.estrellas.clear();
    campo.estrellas.reserve(static_cast<size_t>(cantidad));

    Rng rng(semilla);
    for (int i = 0; i < cantidad; ++i) {
        Estrella estrella;
        estrella.x = rng.entre(-mitadAncho, mitadAncho);
        estrella.y = rng.entre(-mitadAlto, mitadAlto);
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
}

void actualizarCampoEstrellas(CampoEstrellas& campo, float dt) {
    campo.tiempo += dt;

    for (Estrella& estrella : campo.estrellas) {
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
}
