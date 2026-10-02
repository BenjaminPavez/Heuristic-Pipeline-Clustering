import csv
import glob
import math
import os
import time
import gurobipy as gp
from gurobipy import GRB

# Parametros
INPUT_DIR         = 'Instances'
PESOS_FILE        = 'Instances/important_tables.csv'
HEUR_SOLVED_DIR   = 'Solved/Algorithm'
OUTPUT_SOLVED_DIR = 'Solved/Gurobi'
OUTPUT_LOGS_DIR   = 'Logs/Gurobi'
RESUMEN           = 'Solved/Gurobi/resumen_gurobi.csv'

LAMBDA_S = 1
LAMBDA_L = 3
LAMBDA_C = -4
C_MAX    = 50

TIEMPO_LIMITE = 3600
MEM_LIMITE = 0
THREADS = 0
USAR_MIP_START = True
MAX_DAGS = 0

os.makedirs(OUTPUT_SOLVED_DIR, exist_ok=True)
os.makedirs(OUTPUT_LOGS_DIR, exist_ok=True)

COLUMNAS = ["instancia", "n_dags", "dominios_K", "pares_afinidad", "C_max",
            "heur_score", "heur_dominios", "verif_afinidad",
            "estado", "obj", "cota", "gap_pct", "tiempo_s", "dominios_usados",
            "brecha_heur_pct"]


def cargar_pesos(ruta):
    e = {}
    if not ruta or not os.path.exists(ruta):
        return e
    with open(ruta, encoding='utf-8') as f:
        next(f)
        for linea in f:
            linea = linea.strip()
            if linea:
                nombre, peso = linea.split(';')[:2]
                e[nombre] = int(peso)
    return e


def cargar_dags(ruta):
    dags = []
    with open(ruta, encoding='utf-8') as f:
        next(f)
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            nombre, nt, tg, ns, sr = linea.split(';')[:5]
            T = {x for x in tg.split(',') if x}
            S = {x for x in sr.split(',') if x}
            if not T or not S:
                continue
            dags.append({"name": nombre, "T": T, "S": S,
                         "w": int(nt) + int(ns)})
    return dags


def afinidad(a, b, e):
    C = a["T"] & b["T"]
    L = ((a["T"] & b["S"]) | (b["T"] & a["S"])) - C
    S = (a["S"] & b["S"]) - L - C
    return (sum(LAMBDA_C - e.get(t, 0) for t in C)
            + sum(LAMBDA_L + e.get(t, 0) for t in L)
            + sum(LAMBDA_S + e.get(t, 0) for t in S))


def leer_solucion(ruta, dags):
    idx = {d["name"]: i for i, d in enumerate(dags)}
    grupos = {}
    with open(ruta, encoding='utf-8') as f:
        for r in csv.DictReader(f):
            grupos.setdefault(r["cluster_id"].strip(), []).append(idx[r["dag_name"].strip()])
    return sorted(grupos.values(), key=min)


def escribir_fila(fila):
    nuevo = not os.path.exists(RESUMEN)
    with open(RESUMEN, 'a', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=COLUMNAS, lineterminator='\n')
        if nuevo:
            w.writeheader()
        w.writerow({c: fila.get(c, "") for c in COLUMNAS})


def procesar_instancia(filepath, e):
    base = os.path.basename(filepath).replace('.csv', '')
    log_path = os.path.join(OUTPUT_LOGS_DIR, f"{base}.txt")
    sol_path = os.path.join(OUTPUT_SOLVED_DIR, f"{base}_solved.csv")
    heur_path = os.path.join(HEUR_SOLVED_DIR, f"{base}_solved.csv")

    dags = cargar_dags(filepath)
    n = len(dags)
    fila = {"instancia": base, "n_dags": n, "C_max": C_MAX}

    if MAX_DAGS and n > MAX_DAGS:
        print(f"[{base}] omitida (n={n} > MAX_DAGS={MAX_DAGS})")
        return

    if max(d["w"] for d in dags) > C_MAX:
        print(f"[{base}] INFACTIBLE: un DAG pesa más que C_max={C_MAX}")
        fila["estado"] = "INFACTIBLE_DATOS"; escribir_fila(fila); return

    heur_grupos = None
    if os.path.exists(heur_path):
        heur_grupos = leer_solucion(heur_path, dags)
        K = len(heur_grupos)
        fila["heur_dominios"] = K
    else:
        K = math.ceil(sum(d["w"] for d in dags) / C_MAX) + max(2, n // 10)
    K = max(K, math.ceil(sum(d["w"] for d in dags) / C_MAX))
    fila["dominios_K"] = K

    aff = {}
    for i in range(n):
        for j in range(i + 1, n):
            a = afinidad(dags[i], dags[j], e)
            if a != 0:
                aff[i, j] = a
    fila["pares_afinidad"] = len(aff)

    heur_score = None
    if heur_grupos:
        heur_score = sum(aff.get((min(a, b), max(a, b)), 0)
                         for g in heur_grupos for x, a in enumerate(g) for b in g[x + 1:])
        fila["heur_score"] = heur_score
        fila["verif_afinidad"] = "ok"

    print(f"[{base}] n={n} K={K} pares={len(aff):,} heur={heur_score}", flush=True)

    t0 = time.time()
    env = gp.Env(empty=True)
    env.setParam('LogToConsole', 0)
    env.start()
    m = gp.Model(f"Modelo_{base}", env=env)
    m.setParam('TimeLimit', TIEMPO_LIMITE)
    m.setParam('LogFile', log_path)
    if THREADS:
        m.setParam('Threads', THREADS)
    if MEM_LIMITE:
        m.setParam('SoftMemLimit', MEM_LIMITE)

    Y = m.addVars([(i, k) for i in range(n) for k in range(min(i, K - 1) + 1)],
                  vtype=GRB.BINARY, name="Y")
    U = m.addVars(K, vtype=GRB.BINARY, name="U")
    # X continua en [0,1]: con las restricciones de abajo toma valores 0/1 en el óptimo
    X = m.addVars([(i, j, k) for (i, j) in aff for k in range(min(i, K - 1) + 1)],
                  lb=0, ub=1, name="X")

    m.setObjective(gp.quicksum(aff[i, j] * X[i, j, k] for (i, j, k) in X), GRB.MAXIMIZE)
    m.addConstrs((Y.sum(i, '*') == 1 for i in range(n)), name="Unicidad")
    m.addConstrs((gp.quicksum(dags[i]["w"] * Y[i, k] for i in range(k, n))
                  <= C_MAX * U[k] for k in range(K)), name="CapMax")
    m.addConstrs((Y.sum('*', k) >= U[k] for k in range(K)), name="NoTrivial")
  
    for (i, j, k) in X:
        if aff[i, j] > 0:
            m.addConstr(X[i, j, k] <= Y[i, k])
            m.addConstr(X[i, j, k] <= Y[j, k])
        else:
            m.addConstr(X[i, j, k] >= Y[i, k] + Y[j, k] - 1)

    if USAR_MIP_START and heur_grupos:
        for k, miembros in enumerate(heur_grupos):
            for i in miembros:
                if (i, k) in Y:
                    Y[i, k].Start = 1

    m.optimize()

    estados = {GRB.OPTIMAL: "OPTIMO", GRB.TIME_LIMIT: "LIMITE_TIEMPO",
               GRB.INFEASIBLE: "INFACTIBLE", GRB.MEM_LIMIT: "LIMITE_MEMORIA",
               GRB.INTERRUPTED: "INTERRUMPIDO"}
    fila["estado"] = estados.get(m.Status, str(m.Status))
    fila["tiempo_s"] = f"{time.time() - t0:.2f}"

    if m.SolCount > 0:
        asign = {}
        for (i, k), v in Y.items():
            if v.X > 0.5:
                asign.setdefault(k, []).append(i)
        with open(sol_path, 'w', newline='', encoding='utf-8') as f:
            f.write("cluster_id,dag_name\n")
            for nuevo, k in enumerate(sorted(asign)):
                for i in asign[k]:
                    f.write(f"{nuevo},{dags[i]['name']}\n")
        fila["obj"] = f"{m.ObjVal:.0f}"
        fila["cota"] = f"{m.ObjBound:.0f}"
        fila["gap_pct"] = f"{100 * m.MIPGap:.2f}"
        fila["dominios_usados"] = len(asign)
        if heur_score is not None and 0 < m.ObjBound < GRB.INFINITY:
            fila["brecha_heur_pct"] = f"{100 * (m.ObjBound - heur_score) / m.ObjBound:.2f}"
        print(f"[{base}] {fila['estado']} | obj={m.ObjVal:.0f} cota={m.ObjBound:.0f} "
              f"gap={100 * m.MIPGap:.2f}% | heur={heur_score} | {fila['tiempo_s']}s")
    else:
        print(f"[{base}] {fila['estado']} sin solución factible en el tiempo dado")

    escribir_fila(fila)
    m.dispose(); env.dispose()


if __name__ == "__main__":
    e = cargar_pesos(PESOS_FILE)
    print(f"Pesos e(t) cargados: {len(e)} tablas")
    print(f"Parámetros: λL={LAMBDA_L} λS={LAMBDA_S} λC={LAMBDA_C} C_max={C_MAX}\n")

    hechas = set()
    if os.path.exists(RESUMEN):
        with open(RESUMEN, encoding='utf-8') as f:
            hechas = {r["instancia"] for r in csv.DictReader(f)}

    archivos = sorted(glob.glob(os.path.join(INPUT_DIR, 'dags_*.csv')),
                      key=lambda p: os.path.getsize(p))   # de menor a mayor
    print(f"{len(archivos)} instancias; {len(hechas)} ya resueltas se omiten.\n")

    for a in archivos:
        if os.path.basename(a).replace('.csv', '') in hechas:
            continue
        try:
            procesar_instancia(a, e)
        except (gp.GurobiError, MemoryError) as ex:
            base = os.path.basename(a).replace('.csv', '')
            msg = str(ex).replace(',', ';')[:100]
            print(f"[{base}] ERROR: {msg}")
            escribir_fila({"instancia": base, "estado": f"ERROR: {msg}"})