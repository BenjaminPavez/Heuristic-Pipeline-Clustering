#!/usr/bin/env bash
# =============================================================================
# Corre la heurística (Greedy + Tabú, C++) y el modelo exacto (Gurobi) sobre
# todas las instancias de Instances/ y junta los resultados en
# resultados/resumen.csv
#
# Uso:
#   ./tests.sh                      # todas las instancias
#   ./tests.sh dags_010 dags_208    # solo las que contengan esos textos
#
# Opciones (variables de entorno, se ponen antes del comando):
#   TIME_LIMIT=3600  Límite de Gurobi por instancia, en segundos
#   MIP_START=1      Gurobi parte desde la solución de la heurística
#   SOLO_HEUR=1      Corre solo la heurística (útil para dags_300 / dags_500)
#   THREADS=8        Núcleos para Gurobi (0 = todos)
#   MEM_LIMIT=16     Límite de memoria de Gurobi en GB (0 = sin límite)
#   FORZAR=1         Vuelve a correr instancias que ya están en el resumen
#   PYTHON=...       Intérprete de Python (por defecto el de venv/ si existe)
#   SOLVER=...       Script de Gurobi (por defecto solver.py)
#
# Ejemplo:  TIME_LIMIT=600 ./tests.sh dags_010 dags_015
#
# Estructura esperada (todo en la misma carpeta que este script):
#   *.cpp *.h              código de la heurística (no se modifica)
#   solver.py              modelo de Gurobi (con argumentos)
#   venv/                  entorno con gurobipy
#   Instances/dags_*.csv   instancias
#   Instances/important_tables.csv   (opcional, pesos e(t))
# =============================================================================
set -u
cd "$(dirname "$0")"

# --- Python: usa el del venv si existe (Linux/macOS: bin/, Windows: Scripts/)
if [ -z "${PYTHON:-}" ]; then
    for c in venv/bin/python venv/Scripts/python.exe .venv/bin/python .venv/Scripts/python.exe; do
        if [ -x "$c" ]; then PYTHON="$(pwd)/$c"; break; fi
    done
    PYTHON=${PYTHON:-python3}
fi
SOLVER=${SOLVER:-solver.py}
TIME_LIMIT=${TIME_LIMIT:-3600}
MIP_START=${MIP_START:-0}
SOLO_HEUR=${SOLO_HEUR:-0}
THREADS=${THREADS:-0}
MEM_LIMIT=${MEM_LIMIT:-0}
FORZAR=${FORZAR:-0}
DIR_INST=Instances
PESOS="$DIR_INST/important_tables.csv"
SALIDA=resultados
RESUMEN="$SALIDA/resumen.csv"
BIN="$(pwd)/heuristica"
HEADER="instancia,n_dags,dominios_K,pares_afinidad,heur_score,heur_tiempo_s,heur_dominios_con_dags,heur_factible,grb_estado,grb_obj,grb_cota,grb_gap_pct,grb_tiempo_construccion_s,grb_tiempo_solver_s,grb_dominios_con_dags,heur_brecha_vs_cota_pct,verif_afinidad,mip_start"

ahora() { "$PYTHON" -c 'import time; print(f"{time.time():.3f}")'; }
leer_constante() { grep -E "$1" "$2" | head -1 | sed -E 's/.*=[[:space:]]*(-?[0-9]+).*/\1/'; }

# --- Parámetros: se leen del C++ para que ambos métodos usen exactamente los mismos
LAM_S=$(leer_constante 'WEIGHT_SHARED_TABLE' types.h)
LAM_L=$(leer_constante 'WEIGHT_LINEAGE_RELATION' types.h)
LAM_C=$(leer_constante 'WEIGHT_WRITE_CONFLICT' types.h)
CMAX=$(leer_constante 'MAX_CAPACITY[[:space:]]*=' main.cpp)
if [ -z "$LAM_S" ] || [ -z "$LAM_L" ] || [ -z "$LAM_C" ] || [ -z "$CMAX" ]; then
    echo "ERROR: no pude leer los λ de types.h o MAX_CAPACITY de main.cpp"; exit 1
fi
MODO=$([ "$MIP_START" = 1 ] && echo si || echo no)

echo "=============================================================="
echo " Parámetros leídos del C++:  λL=$LAM_L  λS=$LAM_S  λC=$LAM_C  C_max=$CMAX"
echo " Gurobi: TIME_LIMIT=${TIME_LIMIT}s  MIP_START=$MODO  SOLO_HEUR=$SOLO_HEUR"
echo " Python: $PYTHON"
echo " Solver: $SOLVER"
echo "=============================================================="

# --- Compilar la heurística si no existe o si el código cambió
if [ ! -x "$BIN" ] || [ -n "$(find . -maxdepth 1 \( -name '*.cpp' -o -name '*.h' \) -newer "$BIN")" ]; then
    echo "Compilando heurística..."
    g++ -std=c++17 -O2 -o "$BIN" ./*.cpp || { echo "ERROR: no compiló"; exit 1; }
fi

# --- Verificar gurobipy y el solver (solo si se va a usar)
if [ "$SOLO_HEUR" != 1 ]; then
    if ! "$PYTHON" -c "import gurobipy" 2>/dev/null; then
        echo "ERROR: $PYTHON no encuentra gurobipy. ¿Está instalado en el venv?"; exit 1
    fi
    if [ ! -f "$SOLVER" ] || ! grep -q -- '--heur-solucion' "$SOLVER"; then
        echo "ERROR: $SOLVER no es la versión con argumentos (reemplázalo por el solver.py nuevo)"; exit 1
    fi
fi

mkdir -p "$SALIDA"

# Escribe una fila desde bash (cuando Python no la escribe)
fila_bash() {  # $1 = estado
    [ -f "$RESUMEN" ] || echo "$HEADER" > "$RESUMEN"
    echo "$INST,$N,$K,,$SCORE,$T_HEUR,$USADOS,$FACT,$1,,,,,,,,,$MODO" >> "$RESUMEN"
}

# --- Lista de instancias (con filtro opcional)
INSTANCIAS=()
for f in "$DIR_INST"/dags_*.csv; do
    [ -e "$f" ] || continue
    nombre=$(basename "$f" .csv)
    if [ $# -gt 0 ]; then
        ok=0; for filtro in "$@"; do case "$nombre" in *"$filtro"*) ok=1 ;; esac; done
        [ $ok = 1 ] || continue
    fi
    INSTANCIAS+=("$f")
done
[ ${#INSTANCIAS[@]} -gt 0 ] || { echo "No hay instancias que correr."; exit 1; }
echo "Instancias a correr: ${#INSTANCIAS[@]}"

for f in "${INSTANCIAS[@]}"; do
    INST=$(basename "$f" .csv)

    if [ "$FORZAR" != 1 ] && [ -f "$RESUMEN" ] && grep -qE "^${INST},.*,${MODO}\$" "$RESUMEN"; then
        echo "--- $INST ya está en el resumen (MIP_START=$MODO), se omite. Usa FORZAR=1 para repetir."
        continue
    fi
    if [ "$FORZAR" = 1 ] && [ -f "$RESUMEN" ]; then   # reemplazar la fila anterior
        grep -vE "^${INST},.*,${MODO}\$" "$RESUMEN" > "$RESUMEN.tmp"; mv "$RESUMEN.tmp" "$RESUMEN"
    fi
    echo ""
    echo "######## $INST ########"
    OUT="$SALIDA/$INST"
    mkdir -p "$OUT"

    # ---------------- Heurística ----------------
    # main.cpp lee Instances/dags_300.csv: se copia la instancia con ese nombre
    # en una carpeta temporal, sin tocar tus archivos.
    TMP=$(mktemp -d)
    mkdir -p "$TMP/Instances" "$TMP/Solved"
    cp "$f" "$TMP/Instances/dags_300.csv"
    [ -f "$PESOS" ] && cp "$PESOS" "$TMP/Instances/important_tables.csv"

    echo "[heurística] corriendo..."
    t0=$(ahora)
    ( cd "$TMP" && "$BIN" ) > "$OUT/heuristica_consola.txt" 2>&1
    t1=$(ahora)
    T_HEUR=$(awk "BEGIN{printf \"%.2f\", $t1 - $t0}")

    N=$(grep -E '^DAGs cargados:' "$OUT/heuristica_consola.txt" | awk '{print $NF}')
    K=$(grep -E '^Greedy:' "$OUT/heuristica_consola.txt" | awk '{print $2}')
    SCORE=$(grep 'Fitness Global' "$OUT/heuristica_consola.txt" | awk '{print $NF}')
    FACT=$(grep -q 'ERROR' "$OUT/heuristica_consola.txt" && echo no || echo si)
    USADOS=""
    if [ -f "$TMP/Solved/dags_300_solved.csv" ]; then
        cp "$TMP/Solved/dags_300_solved.csv" "$OUT/heuristica_solucion.csv"
        cp "$TMP/Solved/cluster_report.csv"   "$OUT/heuristica_cluster_report.csv"
        cp "$TMP/Solved/critical_tables.csv"  "$OUT/heuristica_critical_tables.csv"
        USADOS=$(tail -n +2 "$OUT/heuristica_solucion.csv" | cut -d, -f1 | sort -u | wc -l | tr -d ' ')
    fi
    rm -rf "$TMP"

    if [ -z "$SCORE" ]; then
        echo "[heurística] FALLÓ (ver $OUT/heuristica_consola.txt)"; FACT=no
    else
        echo "[heurística] score=$SCORE  tiempo=${T_HEUR}s  dominios greedy=$K  con DAGs=$USADOS  factible=$FACT"
    fi

    if [ "$SOLO_HEUR" = 1 ]; then
        fila_bash "NO_EJECUTADO"
        continue
    fi

    # ---------------- Gurobi ----------------
    ARGS=(--dags "$f" --lambdas "$LAM_L" "$LAM_S" "$LAM_C" --cmax "$CMAX"
          --time-limit "$TIME_LIMIT" --threads "$THREADS" --mem-limit "$MEM_LIMIT"
          --resultados "$RESUMEN" --log "$OUT/gurobi.log"
          --solucion-salida "$OUT/gurobi_solucion.csv" --silencioso)
    if [ -f "$PESOS" ]; then ARGS+=(--pesos "$PESOS"); else ARGS+=(--pesos ""); fi
    [ -n "$K" ] && ARGS+=(--dominios "$K")
    if [ -n "$SCORE" ]; then
        ARGS+=(--heur-score "$SCORE" --heur-tiempo "$T_HEUR" --heur-factible "$FACT")
        [ -n "$USADOS" ] && ARGS+=(--heur-dominios-usados "$USADOS")
        [ -f "$OUT/heuristica_solucion.csv" ] && ARGS+=(--heur-solucion "$OUT/heuristica_solucion.csv")
    fi
    [ "$MIP_START" = 1 ] && [ -f "$OUT/heuristica_solucion.csv" ] && ARGS+=(--start "$OUT/heuristica_solucion.csv")

    echo "[gurobi] corriendo (límite ${TIME_LIMIT}s, log en $OUT/gurobi.log)..."
    "$PYTHON" "$SOLVER" "${ARGS[@]}" 2>&1 | tee "$OUT/gurobi_consola.txt"
    if [ "${PIPESTATUS[0]}" -ne 0 ] && ! grep -qE "^${INST},.*,${MODO}\$" "$RESUMEN" 2>/dev/null; then
        fila_bash "ERROR_PYTHON"   # p. ej. el proceso murió por falta de memoria
    fi
done

echo ""
echo "=============================== RESUMEN ==============================="
if command -v column >/dev/null 2>&1; then
    cut -d, -f1,2,3,5,6,9,10,11,12,14,16,17,18 "$RESUMEN" | column -s, -t
else
    cat "$RESUMEN"
fi
echo ""
echo "Resultados completos en $RESUMEN y detalle por instancia en $SALIDA/<instancia>/"