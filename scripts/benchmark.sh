#!/usr/bin/env bash
# Genera mediciones repetibles para el informe. Cada configuracion se ejecuta
# por defecto 10 veces, tal como exige la rubrica. El binario es el mismo para
# modo serial y paralelo; --modo decide si se activan los pragmas OpenMP.
#
# Uso:
#   scripts/benchmark.sh [ruta_al_binario] > resultados.csv
#   python scripts/resumir_benchmark.py resultados.csv > resumen.csv
#
# Variables opcionales:
#   VACAS_LIST       N del experimento principal (default: "1000 3000 5000")
#   HILOS_LIST       hilos paralelos (default: "1 2 4 8")
#   FRAMES           frames internos por medicion (default: 20)
#   REPETICIONES     mediciones por configuracion (default: 10; minimo 10)
#   VACAS_SCHED      N del experimento de schedules (default: 5000)
#   VACAS_CHUNK      N del experimento de false sharing (default: 3000)
#   ESTRELLAS_LIST   M del experimento de estrellas (default: "180 10000")

set -euo pipefail

BINARIO="${1:-build/zipzip}"
if [[ ! -x "$BINARIO" ]]; then
    echo "No se encontro el binario en '$BINARIO'. Compila primero o pasa" \
         "la ruta correcta como primer argumento." >&2
    exit 1
fi

VACAS_LIST="${VACAS_LIST:-1000 3000 5000}"
HILOS_LIST="${HILOS_LIST:-1 2 4 8}"
FRAMES="${FRAMES:-20}"
REPETICIONES="${REPETICIONES:-10}"
VACAS_SCHED="${VACAS_SCHED:-5000}"
VACAS_CHUNK="${VACAS_CHUNK:-3000}"
ESTRELLAS_LIST="${ESTRELLAS_LIST:-180 10000}"
SEMILLA="${SEMILLA:-1234}"

if (( REPETICIONES < 10 )); then
    echo "REPETICIONES debe ser al menos 10 para cumplir la rubrica." >&2
    exit 2
fi

echo "experimento,repeticion,modo,vacas,estrellas,hilos," \
     "estrellas_paralelas,schedule,chunk,semilla,ms_sim_avg,ms_sim_p95," \
     "ms_pasada_a_avg,ms_pasada_b_avg,ms_render_avg,fps" | tr -d ' '

correr() {
    local experimento="$1"
    local repeticion="$2"
    shift 2

    local fila
    fila=$("$BINARIO" --bench --frames "$FRAMES" --semilla "$SEMILLA" \
           "$@" 2>/dev/null)
    printf '%s,%s,%s\n' "$experimento" "$repeticion" "$fila"
}

hilos_max=$(echo "$HILOS_LIST" | tr ' ' '\n' | sort -n | tail -1)

echo "# Experimento 1: serial vs paralelo para varios N" >&2
for vacas in $VACAS_LIST; do
    for repeticion in $(seq 1 "$REPETICIONES"); do
        echo "  N=$vacas repeticion=$repeticion/$REPETICIONES" >&2
        correr speedup "$repeticion" --modo serial --vacas "$vacas"
        for hilos in $HILOS_LIST; do
            correr speedup "$repeticion" --modo paralelo --vacas "$vacas" \
                   --hilos "$hilos" --schedule static
        done
    done
done

echo "# Experimento 2: static vs dynamic vs guided" >&2
for repeticion in $(seq 1 "$REPETICIONES"); do
    correr schedule "$repeticion" --modo serial --vacas "$VACAS_SCHED"
    for schedule in static dynamic guided; do
        correr schedule "$repeticion" --modo paralelo --vacas "$VACAS_SCHED" \
               --hilos "$hilos_max" --schedule "$schedule"
    done
done

echo "# Experimento 3: bloques contiguos vs schedule(static,1)" >&2
for repeticion in $(seq 1 "$REPETICIONES"); do
    correr false_sharing "$repeticion" --modo serial --vacas "$VACAS_CHUNK"
    for chunk in 0 1; do
        correr false_sharing "$repeticion" --modo paralelo \
               --vacas "$VACAS_CHUNK" --hilos "$hilos_max" \
               --schedule static --chunk "$chunk"
    done
done

echo "# Experimento 4: evaluar si conviene paralelizar las estrellas" >&2
for estrellas in $ESTRELLAS_LIST; do
    for repeticion in $(seq 1 "$REPETICIONES"); do
        correr estrellas "$repeticion" --modo serial --vacas 1 \
               --estrellas "$estrellas"
        correr estrellas "$repeticion" --modo paralelo --vacas 1 \
               --estrellas "$estrellas" --hilos "$hilos_max"
        correr estrellas "$repeticion" --modo paralelo --vacas 1 \
               --estrellas "$estrellas" --hilos "$hilos_max" \
               --estrellas-paralelas
    done
done
