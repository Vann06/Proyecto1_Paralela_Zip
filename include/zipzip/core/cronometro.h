#ifndef ZIPZIP_CORE_CRONOMETRO_H
#define ZIPZIP_CORE_CRONOMETRO_H

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <vector>

// Cronometro de proposito general para medir bloques de codigo en
// milisegundos. Usa steady_clock (no SDL_GetPerformanceCounter) para que
// tambien funcione en el modo --bench, que no inicializa SDL.
//
// Uso tipico:
//   Cronometro c;
//   c.iniciar();
//   ... trabajo a medir ...
//   c.detener();          // acumula una muestra
//   c.promedioMs();        // resumen tras varias muestras
class Cronometro {
public:
    void iniciar() {
        inicio_ = std::chrono::steady_clock::now();
    }

    // Detiene el cronometro y guarda la muestra en milisegundos.
    void detener() {
        const auto fin = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> transcurrido =
            fin - inicio_;
        muestras_.push_back(transcurrido.count());
    }

    // Registra una muestra medida por otro medio (por ejemplo, un tramo de
    // codigo que ya devuelve su propio tiempo en milisegundos) sin usar
    // iniciar()/detener().
    void registrarMuestra(double milisegundos) {
        muestras_.push_back(milisegundos);
    }

    void limpiar() { muestras_.clear(); }

    size_t cantidadMuestras() const { return muestras_.size(); }

    double promedioMs() const {
        if (muestras_.empty()) return 0.0;
        double suma = 0.0;
        for (double m : muestras_) suma += m;
        return suma / static_cast<double>(muestras_.size());
    }

    double minimoMs() const {
        if (muestras_.empty()) return 0.0;
        return *std::min_element(muestras_.begin(), muestras_.end());
    }

    double maximoMs() const {
        if (muestras_.empty()) return 0.0;
        return *std::max_element(muestras_.begin(), muestras_.end());
    }

    // Percentil 95: el valor bajo el cual cae el 95% de las muestras.
    // Copia el vector porque nth_element reordena en el lugar.
    double p95Ms() const {
        if (muestras_.empty()) return 0.0;
        std::vector<double> copia = muestras_;
        const size_t indice = static_cast<size_t>(
            0.95 * static_cast<double>(copia.size() - 1));
        std::nth_element(copia.begin(), copia.begin() +
                          static_cast<std::ptrdiff_t>(indice), copia.end());
        return copia[indice];
    }

private:
    std::chrono::steady_clock::time_point inicio_{};
    std::vector<double> muestras_;
};

#endif // ZIPZIP_CORE_CRONOMETRO_H
