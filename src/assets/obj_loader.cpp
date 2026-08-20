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
struct Corner { int v = -1, t = -1, n = -1; };

// Parsea un token de cara: "12", "12/7", "12//4", "12/7/4".
// Los campos ausentes quedan en 0.
void parseCorner(const std::string& tok, int& vi, int& ti, int& ni) {
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

    vi = vals[0];
    ti = vals[1];
    ni = vals[2];
}

// OBJ usa indices base 1. Los negativos son relativos al final del arreglo.
int resolver(int idx, size_t total) {
    if (idx > 0) return idx - 1;
    if (idx < 0) return static_cast<int>(total) + idx;
    return -1;  // campo ausente
}

// Centra el modelo en el origen y lo escala a la dimension pedida.
void normalizarEscala(std::vector<Vec3>& V, float objetivo) {
    if (V.empty()) return;

    Vec3 mn = V[0], mx = V[0];
    for (const Vec3& p : V) {
        mn.x = std::fmin(mn.x, p.x);  mx.x = std::fmax(mx.x, p.x);
        mn.y = std::fmin(mn.y, p.y);  mx.y = std::fmax(mx.y, p.y);
        mn.z = std::fmin(mn.z, p.z);  mx.z = std::fmax(mx.z, p.z);
    }

    Vec3 centro = { (mn.x + mx.x) * 0.5f,
                    (mn.y + mx.y) * 0.5f,
                    (mn.z + mx.z) * 0.5f };

    float ext = std::fmax(mx.x - mn.x, std::fmax(mx.y - mn.y, mx.z - mn.z));
    float esc = (ext > 1e-8f) ? (objetivo / ext) : 1.0f;

    for (Vec3& p : V) {
        p.x = (p.x - centro.x) * esc;
        p.y = (p.y - centro.y) * esc;
        p.z = (p.z - centro.z) * esc;
    }
}

// Normales suaves por vertice, acumulando las normales de cara adyacentes.
// La normal de cara NO se normaliza antes de acumular: su magnitud es el
// doble del area del triangulo, asi las caras grandes pesan mas y el
// sombreado queda mejor.
std::vector<Vec3> calcularNormalesSuaves(const std::vector<Vec3>& V,
                                         const std::vector<Corner>& tris) {
    std::vector<Vec3> suaves(V.size());

    for (size_t i = 0; i + 2 < tris.size(); i += 3) {
        const Vec3& a = V[tris[i    ].v];
        const Vec3& b = V[tris[i + 1].v];
        const Vec3& c = V[tris[i + 2].v];
        Vec3 fn = cruz(resta(b, a), resta(c, a));

        for (int k = 0; k < 3; ++k) {
            Vec3& d = suaves[tris[i + k].v];
            d.x += fn.x;  d.y += fn.y;  d.z += fn.z;
        }
    }

    for (Vec3& n : suaves) {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-8f) { n.x /= len;  n.y /= len;  n.z /= len; }
        else             { n = { 0.0f, 1.0f, 0.0f }; }
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

    std::ifstream f(ruta);
    if (!f) return fallar(error, "No se pudo abrir el archivo: " + ruta);

    std::vector<Vec3>   V;    // posiciones
    std::vector<Vec3>   N;    // normales del archivo
    std::vector<float>  T;    // uv, 2 floats por entrada
    std::vector<Corner> tris; // 3 corners por triangulo

    std::string linea;
    while (std::getline(f, linea)) {
        // Archivos guardados en Windows traen '\r' al final de cada linea.
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        if (linea.empty() || linea[0] == '#') continue;

        std::istringstream ss(linea);
        std::string tipo;
        ss >> tipo;

        if (tipo == "v") {
            Vec3 p;
            ss >> p.x >> p.y >> p.z;
            V.push_back(p);
        }
        else if (tipo == "vn") {
            Vec3 p;
            ss >> p.x >> p.y >> p.z;
            N.push_back(p);
        }
        else if (tipo == "vt") {
            float u = 0.0f, v = 0.0f;
            ss >> u >> v;
            T.push_back(u);
            T.push_back(v);
        }
        else if (tipo == "f") {
            // Una cara puede tener 3, 4 o mas vertices.
            std::vector<Corner> poly;
            std::string tok;
            while (ss >> tok) {
                int a, b, c;
                parseCorner(tok, a, b, c);
                Corner e;
                e.v = resolver(a, V.size());
                e.t = resolver(b, T.size() / 2);
                e.n = resolver(c, N.size());
                if (e.v >= 0 && e.v < static_cast<int>(V.size()))
                    poly.push_back(e);
            }
            // Triangulacion en abanico: (0,1,2), (0,2,3), (0,3,4)...
            // Correcto para poligonos convexos, que es el caso normal.
            for (size_t k = 1; k + 1 < poly.size(); ++k) {
                tris.push_back(poly[0]);
                tris.push_back(poly[k]);
                tris.push_back(poly[k + 1]);
            }
        }
        // mtllib, usemtl, o, g, s: ignorados a proposito
    }

    if (V.empty())    return fallar(error, "El archivo no tiene vertices (v)");
    if (tris.empty()) return fallar(error, "El archivo no tiene caras (f)");

    if (opts.normalizar) normalizarEscala(V, opts.tamanoObjetivo);

    // Usar las normales del archivo solo si estan completas.
    bool faltanNormales = N.empty();
    if (!faltanNormales) {
        for (const Corner& c : tris) {
            if (c.n < 0 || c.n >= static_cast<int>(N.size())) {
                faltanNormales = true;
                break;
            }
        }
    }

    std::vector<Vec3> suaves;
    if (faltanNormales) suaves = calcularNormalesSuaves(V, tris);

    // Aplanar a arrays contiguos, de-indexando.
    out.tieneUV     = !T.empty();
    out.normalesGen = faltanNormales;
    out.triangulos  = static_cast<int>(tris.size()) / 3;
    out.verticesRaw = static_cast<int>(V.size());

    out.pos.clear();
    out.nrm.clear();
    out.uv.clear();
    out.pos.reserve(tris.size() * 3);
    out.nrm.reserve(tris.size() * 3);
    if (out.tieneUV) out.uv.reserve(tris.size() * 2);

    for (const Corner& c : tris) {
        const Vec3& p = V[c.v];
        out.pos.push_back(p.x);
        out.pos.push_back(p.y);
        out.pos.push_back(p.z);

        Vec3 n = (!faltanNormales && c.n >= 0) ? N[c.n] : suaves[c.v];
        out.nrm.push_back(n.x);
        out.nrm.push_back(n.y);
        out.nrm.push_back(n.z);

        if (out.tieneUV) {
            if (c.t >= 0 && static_cast<size_t>(c.t * 2 + 1) < T.size()) {
                out.uv.push_back(T[c.t * 2]);
                out.uv.push_back(T[c.t * 2 + 1]);
            } else {
                out.uv.push_back(0.0f);
                out.uv.push_back(0.0f);
            }
        }
    }

    return true;
}
