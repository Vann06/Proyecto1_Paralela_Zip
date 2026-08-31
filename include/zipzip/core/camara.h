#ifndef ZIPZIP_CORE_CAMARA_H
#define ZIPZIP_CORE_CAMARA_H

#include <cmath>

// Parametros de la camara compartidos entre el renderer y el modo --bench.
//
// El modo --bench no inicializa SDL ni OpenGL (para poder subir la cantidad
// de elementos sin que el render sea el cuello de botella), pero necesita las
// mismas dimensiones de escena (mitadAncho/mitadAlto) que ve la ventana real
// para que la simulacion medida sea la misma. Por eso esta cuenta vive aqui
// y no como metodo de Renderer.

namespace zipzip {

constexpr float PI = 3.14159265358979323846f;
constexpr float FOV_GRAD = 60.0f;
constexpr float DIST_CAM = 4.0f;

// Mitad de la altura visible en el plano z = 0, vista desde DIST_CAM con un
// campo de vision vertical de FOV_GRAD. Pura trigonometria: no depende de
// SDL ni de OpenGL.
inline float mitadAltoVisible() {
    const float fov = FOV_GRAD * PI / 180.0f;
    return DIST_CAM * std::tan(fov * 0.5f);
}

} // namespace zipzip

#endif // ZIPZIP_CORE_CAMARA_H
