#ifndef ESCENA_H
#define ESCENA_H

#include <vector>

// Conjunto de instancias del mismo modelo repartidas en la ventana.
//
// Igual que el loader, este modulo no depende de SDL ni de OpenGL: solo
// mantiene el estado de cada vaca. El render lee ese estado y lo dibuja.
//
// 'actualizarEscena' tiene dos lazos costosos con forma bien distinta:
//   - la interaccion entre vacas (separacion mutua) es O(N^2): cada vaca
//     mira a todas las demas. Es trabajo de CPU real y escala casi lineal
//     con los hilos.
//   - el resto (integracion, rebote, planetas, estrellas) es O(N): trae
//     poco calculo por cada acceso a memoria, asi que su speedup se aplana
//     rapido sin importar cuantos hilos haya.
// El contraste entre ambos es intencional: sirve para mostrar en el informe
// la diferencia entre un kernel memory-bound y uno compute-bound.

struct Instancia {
    float x = 0.0f, y = 0.0f;   // centro, en el plano z = 0
    float giro = 0.0f;          // rotacion actual sobre el eje Y, en grados
    float velGiro = 0.0f;       // grados por segundo
    float escala = 1.0f;        // el modelo llega normalizado a tamano 2
    float r = 1.0f, g = 1.0f, b = 1.0f;

    // Velocidad lineal de la vaca sobre la superficie del semicirculo.
    float vx = 0.0f;
    float vy = 0.0f;
};

struct Planeta {
    float x = 0.0f;
    float y = 0.0f;
    float escala = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    int textura = 0;
    float vx = 0.0f;
    float giro = 0.0f;
    float velGiro = 0.0f;
    bool tieneAro = false;
    float inclinacionAro = 60.0f;

    // Orientacion fija del aro alrededor del planeta, elegida una vez en
    // crearEscena para que cada planeta se vea distinto. A diferencia de
    // 'giro' (el planeta si gira sobre su eje cada frame), este angulo no
    // se anima: es una decision de variedad visual, no de movimiento.
    float anguloAroInicial = 0.0f;
};

// El OVNI sobrevuela la plataforma en una trayectoria de Lissajous; su
// posicion es la unica que las vacas consultan para huir. El movimiento en
// si es O(1) por frame y no afecta la medicion de la parte O(N^2).
struct Ovni {
    float x = 0.0f;
    float y = 0.0f;
    // Desplazamiento respecto al plano z = 0 de las vacas: un valor
    // positivo lo acerca a la camara (queda en frente de las vacas y de la
    // plataforma, y se ve mas grande por la perspectiva) sin desalinearlo
    // en pantalla de su posicion real en la simulacion (que es 2D, solo en
    // x/y).
    float z = 0.9f;

    // Chico a proposito: el z positivo de arriba ya lo pone en frente de
    // las vacas (no se esconde detras por la prueba de profundidad); con
    // esta escala se ve del tamano de algo que sobrevuela la manada, no de
    // un objeto gigante encima de ellas.
    float escala = 0.28f;
    float giro = 0.0f;            // rotacion visual sobre el eje Y
    float velGiro = -50.0f;       // grados por segundo
    float fase = 0.0f;            // acumulador de tiempo para la trayectoria
};

struct Escena {
    std::vector<Instancia> vacas;

    // Mitad del area visible en el plano z = 0. Se calcula desde la camara
    // y sirve para repartir las instancias y, mas adelante, para rebotar.
    float mitadAncho = 1.0f;
    float mitadAlto  = 1.0f;

    // Plataforma circular compartida por simulacion y renderizado: solo la
    // mitad superior del circulo esta en juego (un semicirculo), con su
    // diametro apoyado en el borde inferior de la pantalla -- como el
    // horizonte curvo de un planeta visto desde la superficie. El centro
    // real del circulo (circuloCentroY) queda fuera de pantalla, justo en
    // ese borde; las vacas nunca bajan de ahi.
    float circuloRadio = 1.0f;
    float circuloCentroY = 0.0f;

    std::vector<Planeta> planetas;
    Ovni ovni;

    // Parametros de la interaccion O(N^2) entre vacas.
    //
    // radioSeparacion es un balance entre dos cosas opuestas:
    //   - Muy grande (ej. 0.9 sobre el rombo viejo) y cada vaca cercana al
    //     centro tiene tantas vecinas dentro de rango a la vez que ninguna
    //     fuerza de cohesion razonable puede competir, y la manada termina
    //     amontonada contra el borde.
    //   - Muy chico (0.4, valor anterior) y el kernel O(N^2) se vuelve tan
    //     barato por par que unos pocos miles de vacas no alcanzan a
    //     ocupar a los hilos: en la ventana en vivo (sin VSync, llamando a
    //     actualizarEscena miles de veces por segundo) el overhead de
    //     despertar el equipo de OpenMP terminaba comiendose la ganancia, y
    //     hacia falta subir a ~15,000 vacas para notar el speedup.
    // 0.8 deja la manada bien repartida (verificado con --dump-estado, sin
    // el amontonamiento del primer caso) y ya es visible desde ~3,000-5,000
    // vacas en la ventana, un rango comodo para demostrar la paralelizacion
    // sin tener que esperar a N absurdamente grandes.
    float radioSeparacion = 0.8f;   // distancia a la que dos vacas se repelen
    float fuerzaSeparacion = 2.0f;  // aceleracion maxima de la separacion
    float rapidezMax = 2.6f;        // limite de rapidez tras aplicar fuerzas

    // Cohesion: jaloncito constante hacia un punto comodo dentro de la
    // cupula (no hacia circuloCentroY, que queda fuera de pantalla, sino
    // mas arriba, ver crearEscena), proporcional a que tan lejos esta la
    // vaca de ese punto. Sin esto el modelo solo tendria la fuerza
    // repulsiva de separacion, y una poblacion que solo se repele entre si
    // termina acumulada contra el borde en vez de repartirse por toda la
    // superficie. Es O(1) por vaca (no depende de las demas), asi que se
    // aplica en la pasada B sin tocar el kernel O(N^2).
    float fuerzaCohesion = 1.0f;

    // Punto hacia el que jala la cohesion (ver arriba). Se fija en
    // crearEscena a una altura comoda dentro de la cupula visible.
    float cohesionCentroY = 0.0f;

    // Aceleracion acumulada por vaca en la pasada O(N^2). Vive en la Escena
    // (no es local a la funcion) para no reservar memoria cada frame; su
    // tamano se fija en crearEscena y no cambia mientras corre el programa.
    std::vector<float> ax;
    std::vector<float> ay;

    // Distancia de la vaca mas cercana al OVNI en el ultimo frame; se
    // calcula con una reduccion dentro de la misma pasada O(N^2).
    float distanciaMinimaOvni = 0.0f;
};

// Reparte 'n' instancias dentro del semicirculo. La escala se deriva de una
// grilla virtual para que continue dependiendo de la cantidad de vacas.
void crearEscena(Escena& e, int n,
                 float mitadAncho, float mitadAlto,
                 unsigned semilla = 1234u);

// Avanza vacas, OVNI y planetas. Las vacas se separan entre si (O(N^2));
// luego se integra su movimiento y se reflejan en el borde del semicirculo
// (O(N)).
//
// msPasadaA/msPasadaB son salidas opcionales con el costo de cada pasada por
// separado, en milisegundos. Importan para comparar schedules: el efecto de
// false sharing con schedule(static, 1) vive en la pasada B (escribe en
// Instancia, 40 bytes, menos de una linea de cache), pero la pasada A (el
// O(N^2)) domina el tiempo total y lo diluye si solo se mide la suma.
void actualizarEscena(Escena& e, float dt, bool usarOpenMP,
                      double* msPasadaA = nullptr,
                      double* msPasadaB = nullptr);

#endif // ESCENA_H
