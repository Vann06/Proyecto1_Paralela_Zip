#ifndef ZIPZIP_RENDERING_RENDERER_H
#define ZIPZIP_RENDERING_RENDERER_H

#include "zipzip/assets/obj_loader.h"
#include "zipzip/simulation/scene.h"
#include "zipzip/simulation/starfield.h"

#include <vector>

class Renderer {
public:
    Renderer(int ancho, int alto);

    void inicializar();
    void liberarRecursos();

    void alternarWireframe();
    void alternarCulling();

    bool wireframeActivo() const { return wireframe_; }
    bool cullingActivo() const { return culling_; }

    float mitadAltoVisible() const;
    void dibujar(const Modelo& modeloVaca, const Modelo& modeloOvni,
                 const Escena& escena, const CampoEstrellas& campo,
                 float fps, float msSim, int hilos);

    void dibujarPlanetas(const Escena& escena);

private:
    void configurarProyeccion();
    void prepararTexturasPlanetas();
    void prepararEsferaPlaneta();

    // dibujarEstrellas/dibujarPlanetas/dibujarHUD apagan luces, prueba de
    // profundidad, culling y relleno para dibujar su capa (fondo 2D,
    // texturas translucidas, overlay ortografico); este metodo repite la
    // misma restauracion al final de las tres en vez de repetir las 4
    // lineas cada vez.
    void restaurarEstadoRender();

    void dibujarEstrellas(const CampoEstrellas& campo);
    void dibujarPlataforma(const Escena& escena);
    void dibujarVacas(const Modelo& modelo, const Escena& escena);
    void dibujarOvni(const Modelo& modeloOvni, const Escena& escena);
    void dibujarHUD(float fps, int vacas, int estrellas,
                     float msSim, int hilos, float distMinOvni);

    int ancho_;
    int alto_;
    bool wireframe_ = false;
    bool culling_ = false;
    unsigned int texturasPlanetas_[4] = {};
    bool texturasPlanetasPreparadas_ = false;

    // Geometria de una esfera unitaria (UV + posicion, 5 floats por
    // vertice, 4 vertices por quad), calculada una sola vez en inicializar()
    // en vez de recomputar seno/coseno para cada planeta en cada frame.
    std::vector<float> esferaPlaneta_;
};

#endif // ZIPZIP_RENDERING_RENDERER_H
