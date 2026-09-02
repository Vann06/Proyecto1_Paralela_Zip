#ifndef ZIPZIP_CORE_RNG_H
#define ZIPZIP_CORE_RNG_H

#include <cstdint>

// Generador propio (xorshift32) en vez de <random>: determinista entre
// maquinas y compiladores, asi la misma semilla siempre da la misma escena
// (clave para comparar serial vs. paralelo con --dump-estado).

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
