#!/bin/bash

if [ $# -gt 0 ]; then
    SEEDS=("$@")
else
    SEEDS=(1 2 3 4 5)
fi

BIN=./dag_clusterer


echo "Compilando"
make > /dev/null
if [ $? -ne 0 ] || [ ! -x "$BIN" ]; then
    echo "ERROR: no se pudo compilar $BIN"
    exit 1
fi

for seed in "${SEEDS[@]}"
do
    LOG_DIR="Logs/seed_${seed}"
    OUT_DIR="Solved/Algorithm/seed_${seed}"
    mkdir -p "$LOG_DIR" "$OUT_DIR"

    echo "----------------------------------------"
    echo "Semilla: $seed"
    echo "----------------------------------------"

    seed_start=$(date +%s.%N)

    for file in Instances/dags_*.csv
    do
        instance=$(basename "$file" .csv)
        echo "  [seed $seed] $instance"

        "$BIN" "$file" "$seed" "$OUT_DIR" > "${LOG_DIR}/${instance}.txt" 2>&1

        if [ $? -ne 0 ]; then
            echo "  ERROR al ejecutar: $file (semilla $seed)"
            echo "  Revisa: ${LOG_DIR}/${instance}.txt"
        fi
    done

    seed_end=$(date +%s.%N)
    total=$(echo "$seed_end - $seed_start" | bc)
    printf "Semilla %s: tiempo total %.1f segundos (%.1f minutos)\n" \
        "$seed" "$total" "$(echo "$total / 60" | bc -l)" | tee "${LOG_DIR}/_tiempo_total.txt"
done

echo "========================================"
echo "Ejecucion terminada para las semillas: ${SEEDS[*]}"
echo "Logs en Logs/seed_<s>/ y soluciones en Solved/Algorithm/seed_<s>/"
echo "========================================"