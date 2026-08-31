#include "zipzip/assets/obj_loader.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

// Todo lo que va en este namespace anonimo es privado del archivo.
// No aparece en el header ni en la tabla de simbolos del ejecutable.
namespace {

struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

Vec3 resta(const Vec3& a, const Vec3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 cruz(const Vec3& a, const Vec3& b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}

// Una esquina de triangulo: indices a posicion / uv / normal.
struct Corner {
    int indicePosicion = -1;
    int indiceUV = -1;
    int indiceNormal = -1;
};

// Parsea un token de cara: "12", "12/7", "12//4", "12/7/4".
// Los campos ausentes quedan en 0.
void parseCorner(const std::string& tok, int& indicePosicion, int& indiceUV,
                  int& indiceNormal) {
    int vals[3] = { 0, 0, 0 };
    const char* p = tok.c_str();
    int slot = 0;

    while (slot < 3) {
        char* fin = nullptr;
        long v = std::strtol(p, &fin, 10);
        if (fin != p) vals[slot] = static_cast<int>(v);
        p = fin;
        if (*p == '/') { ++p; ++slot; }
        else break;
    }

    indicePosicion = vals[0];
    indiceUV = vals[1];
    indiceNormal = vals[2];
}

// OBJ usa indices base 1. Los negativos son relativos al final del arreglo.
int resolver(int idx, size_t total) {
    if (idx > 0) return idx - 1;
    if (idx < 0) return static_cast<int>(total) + idx;
    return -1;  // campo ausente
}

// Centra el modelo en el origen y lo escala a la dimension pedida.
void normalizarEscala(std::vector<Vec3>& posiciones, float objetivo) {
    if (posiciones.empty()) return;

    Vec3 minimo = posiciones[0], maximo = posiciones[0];
    for (const Vec3& punto : posiciones) {
        minimo.x = std::fmin(minimo.x, punto.x);
        maximo.x = std::fmax(maximo.x, punto.x);
        minimo.y = std::fmin(minimo.y, punto.y);
        maximo.y = std::fmax(maximo.y, punto.y);
        minimo.z = std::fmin(minimo.z, punto.z);
        maximo.z = std::fmax(maximo.z, punto.z);
    }

    Vec3 centro = { (minimo.x + maximo.x) * 0.5f,
                    (minimo.y + maximo.y) * 0.5f,
                    (minimo.z + maximo.z) * 0.5f };

    float extension = std::fmax(maximo.x - minimo.x,
                                std::fmax(maximo.y - minimo.y,
                                          maximo.z - minimo.z));
    float escala = (extension > 1e-8f) ? (objetivo / extension) : 1.0f;

    for (Vec3& punto : posiciones) {
        punto.x = (punto.x - centro.x) * escala;
        punto.y = (punto.y - centro.y) * escala;
        punto.z = (punto.z - centro.z) * escala;
    }
}

// Normales suaves por vertice, acumulando las normales de cara adyacentes.
// La normal de cara NO se normaliza antes de acumular: su magnitud es el
// doble del area del triangulo, asi las caras grandes pesan mas y el
// sombreado queda mejor.
std::vector<Vec3> calcularNormalesSuaves(const std::vector<Vec3>& posiciones,
                                         const std::vector<Corner>& tris) {
    std::vector<Vec3> suaves(posiciones.size());

    for (size_t i = 0; i + 2 < tris.size(); i += 3) {
        const Vec3& verticeA = posiciones[tris[i    ].indicePosicion];
        const Vec3& verticeB = posiciones[tris[i + 1].indicePosicion];
        const Vec3& verticeC = posiciones[tris[i + 2].indicePosicion];
        Vec3 normalDeCara = cruz(resta(verticeB, verticeA),
                                 resta(verticeC, verticeA));

        for (int indiceEsquina = 0; indiceEsquina < 3; ++indiceEsquina) {
            Vec3& normalAcumulada =
                suaves[tris[i + indiceEsquina].indicePosicion];
            normalAcumulada.x += normalDeCara.x;
            normalAcumulada.y += normalDeCara.y;
            normalAcumulada.z += normalDeCara.z;
        }
    }

    for (Vec3& normal : suaves) {
        float longitud = std::sqrt(
            normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (longitud > 1e-8f) {
            normal.x /= longitud;
            normal.y /= longitud;
            normal.z /= longitud;
        } else {
            normal = { 0.0f, 1.0f, 0.0f };
        }
    }

    return suaves;
}

// Helper para reportar errores sin repetir el chequeo de puntero nulo.
bool fallar(std::string* destino, const std::string& msg) {
    if (destino) *destino = msg;
    return false;
}

} // namespace anonimo

// ---------------------------------------------------------------- interfaz

bool cargarOBJ(const std::string& ruta,
               Modelo& out,
               const OpcionesOBJ& opts,
               std::string* error) {

    std::ifstream archivo(ruta);
    if (!archivo) return fallar(error, "No se pudo abrir el archivo: " + ruta);

    std::vector<Vec3>   posiciones;      // posiciones ("v") leidas del archivo
    std::vector<Vec3>   normalesArchivo; // normales ("vn") leidas del archivo
    std::vector<float>  coordenadasUV;   // uv ("vt"), 2 floats por entrada
    std::vector<Corner> tris;            // 3 corners por triangulo
    std::vector<std::string> gruposTriangulos;
    std::string grupoActual = "default";

    std::string linea;
    while (std::getline(archivo, linea)) {
        // Archivos guardados en Windows traen '\r' al final de cada linea.
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        if (linea.empty() || linea[0] == '#') continue;

        std::istringstream ss(linea);
        std::string tipo;
        ss >> tipo;

        if (tipo == "v") {
            Vec3 punto;
            ss >> punto.x >> punto.y >> punto.z;
            posiciones.push_back(punto);
        }
        else if (tipo == "vn") {
            Vec3 punto;
            ss >> punto.x >> punto.y >> punto.z;
            normalesArchivo.push_back(punto);
        }
        else if (tipo == "vt") {
            float u = 0.0f, v = 0.0f;
            ss >> u >> v;
            coordenadasUV.push_back(u);
            coordenadasUV.push_back(v);
        }
        else if (tipo == "g" || tipo == "o") {
            std::string nombre;
            ss >> nombre;
            grupoActual = nombre.empty() ? "default" : nombre;
        }
        else if (tipo == "f") {
            // Una cara puede tener 3, 4 o mas vertices.
            std::vector<Corner> poly;
            std::string tok;
            while (ss >> tok) {
                int indiceCrudoPosicion = 0;
                int indiceCrudoUV = 0;
                int indiceCrudoNormal = 0;
                parseCorner(tok, indiceCrudoPosicion, indiceCrudoUV,
                            indiceCrudoNormal);
                Corner esquina;
                esquina.indicePosicion =
                    resolver(indiceCrudoPosicion, posiciones.size());
                esquina.indiceUV =
                    resolver(indiceCrudoUV, coordenadasUV.size() / 2);
                esquina.indiceNormal =
                    resolver(indiceCrudoNormal, normalesArchivo.size());
                if (esquina.indicePosicion >= 0 &&
                    esquina.indicePosicion < static_cast<int>(posiciones.size()))
                    poly.push_back(esquina);
            }
            // Triangulacion en abanico: (0,1,2), (0,2,3), (0,3,4)...
            // Correcto para poligonos convexos, que es el caso normal.
            for (size_t indiceAbanico = 1; indiceAbanico + 1 < poly.size();
                 ++indiceAbanico) {
                tris.push_back(poly[0]);
                tris.push_back(poly[indiceAbanico]);
                tris.push_back(poly[indiceAbanico + 1]);
                gruposTriangulos.push_back(grupoActual);
            }
        }
        // mtllib, usemtl y s se ignoran; los grupos g/o se conservan.
    }

    if (posiciones.empty()) {
        return fallar(error, "El archivo no tiene vertices (v)");
    }
    if (tris.empty()) return fallar(error, "El archivo no tiene caras (f)");

    if (opts.normalizar) normalizarEscala(posiciones, opts.tamanoObjetivo);

    // Usar las normales del archivo solo si estan completas.
    bool faltanNormales = normalesArchivo.empty();
    if (!faltanNormales) {
        for (const Corner& esquina : tris) {
            if (esquina.indiceNormal < 0 ||
                esquina.indiceNormal >= static_cast<int>(normalesArchivo.size())) {
                faltanNormales = true;
                break;
            }
        }
    }

    std::vector<Vec3> suaves;
    if (faltanNormales) suaves = calcularNormalesSuaves(posiciones, tris);

    // Aplanar a arrays contiguos, de-indexando.
    out.tieneUV     = !coordenadasUV.empty();
    out.normalesGen = faltanNormales;
    out.triangulos  = static_cast<int>(tris.size()) / 3;
    out.verticesRaw = static_cast<int>(posiciones.size());

    out.pos.clear();
    out.nrm.clear();
    out.uv.clear();
    out.rangos.clear();
    out.pos.reserve(tris.size() * 3);
    out.nrm.reserve(tris.size() * 3);
    if (out.tieneUV) out.uv.reserve(tris.size() * 2);

    for (size_t indiceTriangulo = 0;
         indiceTriangulo < gruposTriangulos.size(); ++indiceTriangulo) {
        const std::string& nombre = gruposTriangulos[indiceTriangulo];
        if (out.rangos.empty() || out.rangos.back().nombre != nombre) {
            RangoModelo rango;
            rango.nombre = nombre;
            rango.primerVertice = static_cast<int>(indiceTriangulo * 3);
            rango.cantidadVertices = 3;
            out.rangos.push_back(rango);
        } else {
            out.rangos.back().cantidadVertices += 3;
        }
    }

    for (const Corner& esquina : tris) {
        const Vec3& punto = posiciones[esquina.indicePosicion];
        out.pos.push_back(punto.x);
        out.pos.push_back(punto.y);
        out.pos.push_back(punto.z);

        Vec3 normal = (!faltanNormales && esquina.indiceNormal >= 0)
            ? normalesArchivo[esquina.indiceNormal]
            : suaves[esquina.indicePosicion];
        out.nrm.push_back(normal.x);
        out.nrm.push_back(normal.y);
        out.nrm.push_back(normal.z);

        if (out.tieneUV) {
            if (esquina.indiceUV >= 0 &&
                static_cast<size_t>(esquina.indiceUV * 2 + 1) <
                    coordenadasUV.size()) {
                out.uv.push_back(coordenadasUV[esquina.indiceUV * 2]);
                out.uv.push_back(coordenadasUV[esquina.indiceUV * 2 + 1]);
            } else {
                out.uv.push_back(0.0f);
                out.uv.push_back(0.0f);
            }
        }
    }

    return true;
}
