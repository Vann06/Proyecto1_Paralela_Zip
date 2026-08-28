#ifndef ZIPZIP_RENDERING_RENDERER_H
#define ZIPZIP_RENDERING_RENDERER_H

#include "zipzip/assets/obj_loader.h"
#include "zipzip/simulation/scene.h"
#include "zipzip/simulation/starfield.h"

class Renderer {
public:
    Renderer(int ancho, int alto);

    void inicializar();
    void prepararModelo(const Modelo& modelo);
    void liberarModelo();

    void alternarWireframe();
    void alternarCulling();

    bool wireframeActivo() const { return wireframe_; }
    bool cullingActivo() const { return culling_; }

    float mitadAltoVisible() const;
    void dibujar(const Modelo& modelo, const Escena& escena,
                 const CampoEstrellas& campo, float fps);

    void dibujarPlanetas(const Escena& escena);

private:
    void configurarProyeccion();
    void dibujarEstrellas(const CampoEstrellas& campo);
    void dibujarVacas(const Modelo& modelo, const Escena& escena);
    void dibujarHUD(float fps, int vacas, int estrellas);

    int ancho_;
    int alto_;
    bool wireframe_ = false;
    bool culling_ = false;
};

#endif // ZIPZIP_RENDERING_RENDERER_H
