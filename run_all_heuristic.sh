#!/bin/bash

mkdir -p Logs

for file in Instances/dags_*.csv
do
    instance=$(basename "$file" .csv)

    echo "========================================"
    echo "Ejecutando: $file"
    echo "Log: Logs/${instance}.txt"
    echo "========================================"

    make run FILE="$file" > "Logs/${instance}.txt" 2>&1

    if [ $? -ne 0 ]; then
        echo "ERROR al ejecutar: $file"
        echo "Revisa: Logs/${instance}.txt"
        continue
    fi
done

echo "========================================"
echo "Todas las instancias fueron ejecutadas."
echo "Los logs están en Logs/"
echo "========================================"