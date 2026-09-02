#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <string>
#include <vector>

// Cargador minimo de archivos Wavefront OBJ. No depende de SDL ni de
// OpenGL: solo produce arrays de floats listos para subir a la GPU.

// Malla lista para dibujar: arrays planos, sin indices.
// Cada triangulo ocupa 3 vertices consecutivos, asi que se puede pasar
// directo a glDrawArrays(GL_TRIANGLES, 0, triangulos * 3).
struct RangoModelo {
    std::string nombre;
    int primerVertice = 0;
    int cantidadVertices = 0;
};

struct Modelo {
    std::vector<float> pos;   // 3 floats por vertice (x, y, z)
    std::vector<float> nrm;   // 3 floats por vertice (nx, ny, nz)
    std::vector<float> uv;    // 2 floats por vertice, vacio si el OBJ no traia vt
    std::vector<RangoModelo> rangos; // grupos 'g'/'o' contiguos del OBJ

    bool tieneUV     = false;  // el archivo traia coordenadas de textura
    bool normalesGen = false;  // true si las normales las calculamos nosotros
    int  triangulos  = 0;      // cantidad de triangulos tras triangular
    int  verticesRaw = 0;      // vertices unicos leidos del archivo
};

// Opciones de carga.
struct OpcionesOBJ {
    // Centra el modelo en el origen y lo escala para que su dimension mayor
    // mida 'tamanoObjetivo'. Sin esto, un modelo en milimetros o descentrado
    // puede quedar fuera del frustum y no verse.
    bool  normalizar     = true;
    float tamanoObjetivo = 2.0f;
};

// Devuelve true si la carga tuvo exito.
// Si falla y 'error' no es nulo, ahi queda el motivo.
bool cargarOBJ(const std::string& ruta,
               Modelo& out,
               const OpcionesOBJ& opts = OpcionesOBJ(),
               std::string* error = nullptr);

#endif // OBJ_LOADER_H
