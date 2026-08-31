#!/usr/bin/env python3
"""Resume resultados.csv y calcula speedup y eficiencia por configuracion."""

from __future__ import annotations

import csv
import statistics
import sys
from collections import defaultdict


CAMPOS_CLAVE = (
    "experimento",
    "modo",
    "vacas",
    "estrellas",
    "hilos",
    "estrellas_paralelas",
    "schedule",
    "chunk",
    "semilla",
)


def leer_filas(ruta: str) -> list[dict[str, str]]:
    with open(ruta, newline="", encoding="utf-8") as archivo:
        filas = list(csv.DictReader(archivo))
    if not filas:
        raise ValueError("El CSV no contiene mediciones")
    faltantes = set(CAMPOS_CLAVE + ("ms_sim_avg",)) - set(filas[0])
    if faltantes:
        raise ValueError(f"Faltan columnas: {', '.join(sorted(faltantes))}")
    return filas


def clave_configuracion(fila: dict[str, str]) -> tuple[str, ...]:
    return tuple(fila[campo] for campo in CAMPOS_CLAVE)


def clave_base(fila: dict[str, str]) -> tuple[str, str, str, str]:
    return (
        fila["experimento"],
        fila["vacas"],
        fila["estrellas"],
        fila["semilla"],
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("Uso: resumir_benchmark.py resultados.csv", file=sys.stderr)
        return 2

    filas = leer_filas(sys.argv[1])
    grupos: dict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
    for fila in filas:
        grupos[clave_configuracion(fila)].append(fila)

    bases: dict[tuple[str, str, str, str], float] = {}
    for mediciones in grupos.values():
        primera = mediciones[0]
        if primera["modo"] == "serial":
            bases[clave_base(primera)] = statistics.mean(
                float(fila["ms_sim_avg"]) for fila in mediciones
            )

    columnas = list(CAMPOS_CLAVE) + [
        "mediciones",
        "ms_sim_promedio",
        "ms_sim_desviacion",
        "speedup",
        "eficiencia",
    ]
    escritor = csv.DictWriter(sys.stdout, fieldnames=columnas, lineterminator="\n")
    escritor.writeheader()

    for clave in sorted(grupos):
        mediciones = grupos[clave]
        primera = mediciones[0]
        tiempos = [float(fila["ms_sim_avg"]) for fila in mediciones]
        promedio = statistics.mean(tiempos)
        desviacion = statistics.stdev(tiempos) if len(tiempos) > 1 else 0.0
        base = bases.get(clave_base(primera))
        if base is None or promedio <= 0.0:
            speedup = 0.0
            eficiencia = 0.0
        else:
            speedup = base / promedio
            hilos = int(primera["hilos"])
            eficiencia = speedup / hilos

        salida = {campo: valor for campo, valor in zip(CAMPOS_CLAVE, clave)}
        salida.update(
            mediciones=str(len(mediciones)),
            ms_sim_promedio=f"{promedio:.6f}",
            ms_sim_desviacion=f"{desviacion:.6f}",
            speedup=f"{speedup:.6f}",
            eficiencia=f"{eficiencia:.6f}",
        )
        escritor.writerow(salida)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1)
