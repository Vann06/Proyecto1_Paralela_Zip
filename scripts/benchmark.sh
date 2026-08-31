#!/usr/bin/env bash
# Corre el modo --bench en un barrido de cantidad de vacas x hilos, mas dos
# barridos pequenos para los experimentos de schedule y false sharing, y
# junta todo en un solo CSV a stdout. Pensado para alimentar directamente la
# tabla y las graficas del informe (speedup, eficiencia).
#
# Uso:
#   scripts/benchmark.sh [ruta_al_binario] > resultados.csv
#
# Variables de entorno opcionales:
#   VACAS_LIST    Lista de N para el barrido principal (default: "1000 2000 5000 10000")
#   HILOS_LIST    Lista de hilos para el barrido principal (default: "1 2 4 8 16")
#   FRAMES        Frames medidos por corrida, sin contar el calentamiento (default: 20)
#   VACAS_CHUNK   N usado en el experimento de false sharing (default: 3000)
#   VACAS_SCHED   N usado en el experimento de schedule (default: 8000)
#
# El encabezado del CSV lo imprime este script (una sola vez); cada corrida
# de --bench manda su encabezado a stderr, no a stdout, para poder
# concatenar filas sin repetirlo.

set -euo pipefail

BINARIO="${1:-build/zipzip}"
if [[ ! -x "$BINARIO" ]]; then
    echo "No se encontro el binario en '$BINARIO'. Compila primero (ver README) o" \
         "pasa la ruta como primer argumento." >&2
    exit 1
fi

VACAS_LIST="${VACAS_LIST:-1000 2000 5000 10000}"
HILOS_LIST="${HILOS_LIST:-1 2 4 8 16}"
FRAMES="${FRAMES:-20}"
VACAS_CHUNK="${VACAS_CHUNK:-3000}"
VACAS_SCHED="${VACAS_SCHED:-8000}"

echo "vacas,estrellas,hilos,schedule,chunk,ms_sim_avg,ms_sim_p95," \
     "ms_pasada_a_avg,ms_pasada_b_avg,ms_render_avg,fps" | tr -d ' '

correr() {
    # Los argumentos de --bench van despues del nombre; el CSV de la corrida
    # sale por stdout, su encabezado por stderr (se descarta con 2>/dev/null).
    "$BINARIO" --bench --frames "$FRAMES" "$@" 2>/dev/null
}

echo "# Experimento 1: speedup y eficiencia (vacas x hilos, schedule static)" >&2
for vacas in $VACAS_LIST; do
    for hilos in $HILOS_LIST; do
        echo "  vacas=$vacas hilos=$hilos" >&2
        correr --vacas "$vacas" --hilos "$hilos" --schedule static
    done
done

echo "# Experimento 2: schedule static vs dynamic vs guided (N=$VACAS_SCHED, hilos maximos)" >&2
hilos_max=$(echo "$HILOS_LIST" | tr ' ' '\n' | sort -n | tail -1)
for sched in static dynamic guided; do
    echo "  schedule=$sched" >&2
    correr --vacas "$VACAS_SCHED" --hilos "$hilos_max" --schedule "$sched"
done

echo "# Experimento 3: false sharing, schedule(static) por bloques vs schedule(static,1)" \
     "(N=$VACAS_CHUNK, hilos maximos; el efecto se ve en ms_pasada_b_avg)" >&2
for chunk in 0 1; do
    echo "  chunk=$chunk" >&2
    correr --vacas "$VACAS_CHUNK" --hilos "$hilos_max" --schedule static --chunk "$chunk"
done
