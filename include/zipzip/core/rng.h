#ifndef ZIPZIP_CORE_RNG_H
#define ZIPZIP_CORE_RNG_H

#include <cstdint>

// Generador propio (xorshift32) en lugar de <random>: es determinista entre
// maquinas y compiladores, asi la misma semilla da siempre la misma escena.
// Eso importa para comparar tiempos entre la version serial y la paralela, y
// para verificar con --dump-estado que ambas producen exactamente el mismo
// resultado.
//
// Vivia triplicado: una copia en scene.cpp, otra en starfield.cpp, y una
// tercera como funciones libres para EstrellaFugaz (que guarda su semilla
// como campo propio en vez de un Rng completo). Las tres comparten el mismo
// algoritmo, asi que queda aqui una sola vez.

namespace zipzip {

inline uint32_t xorshift32(uint32_t& estado) {
    estado ^= estado << 13;
    estado ^= estado >> 17;
    estado ^= estado << 5;
    return estado;
}

// Real uniforme en [a, b) a partir de un estado de xorshift32.
inline float uniformeEntre(uint32_t& estado, float a, float b) {
    return a + (b - a) *
        (static_cast<float>(xorshift32(estado)) / 4294967296.0f);
}

// Envoltorio con estado propio, para el caso comun de un generador por
// escena o campo (no uno por instancia individual).
struct Rng {
    uint32_t estado;
    explicit Rng(uint32_t semilla) : estado(semilla ? semilla : 1u) {}

    uint32_t siguiente() { return xorshift32(estado); }
    float entre(float a, float b) { return uniformeEntre(estado, a, b); }
};

} // namespace zipzip

#endif // ZIPZIP_CORE_RNG_H
