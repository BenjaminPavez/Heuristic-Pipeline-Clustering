"""
Modelo exacto (MIP) del agrupamiento de DAGs, equivalente al del Capítulo 1.
Lee los mismos CSV que el C++ y usa la misma función de afinidad, por lo que
el valor objetivo es directamente comparable con el Score de la heurística.

Uso individual:
    python3 solver.py --dags Instances/dags_050_s1.csv --lambdas 18 2 -16 --cmax 50
Uso por lotes: ver tests.sh
"""
import argparse
import csv
import math
import os
import time
import gurobipy as gp
from gurobipy import GRB


# ----------------------------------------------------------------------------
# Carga de datos (mismo formato y mismo filtro de integridad que utils.cpp)
# ----------------------------------------------------------------------------
def cargar_dags(ruta):
    dags = []
    with open(ruta, encoding="utf-8") as f:
        next(f)                                    # encabezado
        for linea in f:
            linea = linea.rstrip("\n\r")
            if not linea:
                continue
            nombre, n_t, targets, n_s, sources = linea.split(";")[:5]
            T = [t for t in targets.split(",") if t]
            S = [s for s in sources.split(",") if s]
            if not T or not S:                     # check_dag_integrity
                continue
            dags.append({"name": nombre, "T": set(T), "S": set(S),
                         "w": int(n_t) + int(n_s)})  # mismo peso que el C++
    return dags


def cargar_pesos(ruta):
    pesos = {}
    if not ruta or not os.path.exists(ruta):
        return pesos
    with open(ruta, encoding="utf-8") as f:
        next(f)
        for linea in f:
            linea = linea.strip()
            if linea:
                nombre, peso = linea.split(";")[:2]
                pesos[nombre] = int(peso)
    return pesos


def leer_solucion(ruta, dags):
    """Lee un CSV cluster_id,dag_name y devuelve la lista de dominios (índices)."""
    idx = {d["name"]: i for i, d in enumerate(dags)}
    grupos = {}
    with open(ruta, encoding="utf-8") as f:
        for r in csv.DictReader(f):
            grupos.setdefault(r["cluster_id"].strip(), []).append(idx[r["dag_name"].strip()])
    return sorted(grupos.values(), key=min)


def afinidad(a, b, e, lam):
    """Ecuación (15) con conjuntos disjuntos: igual a calculate_affinity()."""
    lam_L, lam_S, lam_C = lam
    C = a["T"] & b["T"]
    L = ((a["T"] & b["S"]) | (b["T"] & a["S"])) - C
    S = (a["S"] & b["S"]) - L - C
    return (sum(lam_C - e.get(t, 0) for t in C)
            + sum(lam_L + e.get(t, 0) for t in L)
            + sum(lam_S + e.get(t, 0) for t in S))


def dominios_greedy(dags, e, lam, cmax):
    """Replica run_greedy() para contar cuántos dominios abre (si no se entrega K)."""
    libres, abiertos = set(range(len(dags))), 0
    while libres:
        dom, cap = [], 0
        while True:
            mejor, mejor_val = -1, -math.inf
            for i in sorted(libres):
                if dom:
                    val = sum(afinidad(dags[d], dags[i], e, lam) for d in dom)
                else:
                    val = dags[i]["w"] + sum(e.get(t, 0) for t in dags[i]["S"]) \
                                        + sum(e.get(t, 0) for t in dags[i]["T"])
                if val > mejor_val and cap + dags[i]["w"] <= cmax:
                    mejor, mejor_val = i, val
            if mejor == -1:
                break
            dom.append(mejor); cap += dags[mejor]["w"]; libres.discard(mejor)
        if not dom:
            raise ValueError("Hay DAGs con peso mayor que C_max: modelo infactible")
        abiertos += 1
    return abiertos


# ----------------------------------------------------------------------------
# Resultados
# ----------------------------------------------------------------------------
COLUMNAS = ["instancia", "n_dags", "dominios_K", "pares_afinidad",
            "heur_score", "heur_tiempo_s", "heur_dominios_con_dags", "heur_factible",
            "grb_estado", "grb_obj", "grb_cota", "grb_gap_pct",
            "grb_tiempo_construccion_s", "grb_tiempo_solver_s", "grb_dominios_con_dags",
            "heur_brecha_vs_cota_pct", "verif_afinidad", "mip_start"]


def escribir_fila(ruta, fila):
    if not ruta:
        return
    nuevo = not os.path.exists(ruta)
    with open(ruta, "a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=COLUMNAS, lineterminator="\n")
        if nuevo:
            w.writeheader()
        w.writerow({c: fila.get(c, "") for c in COLUMNAS})


def fmt(x, dec=2):
    return "" if x is None else (f"{x:.{dec}f}" if isinstance(x, float) else str(x))


# ----------------------------------------------------------------------------
# Modelo
# ----------------------------------------------------------------------------
def resolver(args):
    lam = tuple(args.lambdas)
    dags = cargar_dags(args.dags)
    e = cargar_pesos(args.pesos)
    n = len(dags)
    nombre = os.path.splitext(os.path.basename(args.dags))[0]

    fila = {"instancia": nombre, "n_dags": n,
            "heur_score": fmt(args.heur_score, 0), "heur_tiempo_s": fmt(args.heur_tiempo),
            "heur_dominios_con_dags": fmt(args.heur_dominios_usados),
            "heur_factible": fmt(args.heur_factible), "mip_start": "si" if args.start else "no"}

    t0 = time.time()
    K = args.dominios or dominios_greedy(dags, e, lam, args.cmax)
    fila["dominios_K"] = K
    cota_bp = math.ceil(sum(d["w"] for d in dags) / args.cmax)
    if K < cota_bp:
        raise ValueError(f"K={K} dominios no alcanzan: se necesitan al menos {cota_bp}")

    # Solo pares no ordenados (i<j) con afinidad distinta de cero
    aff = {}
    for i in range(n):
        for j in range(i + 1, n):
            a = afinidad(dags[i], dags[j], e, lam)
            if a != 0:
                aff[i, j] = a
    fila["pares_afinidad"] = len(aff)

    # Verificación: la solución de la heurística evaluada con la afinidad de Python
    # debe dar exactamente el score del C++. Si no, los scores no son comparables.
    sol_heur = args.heur_solucion or args.start
    if sol_heur and args.heur_score is not None and os.path.exists(sol_heur):
        try:
            grupos = leer_solucion(sol_heur, dags)
            score_py = sum(aff.get((min(a, b), max(a, b)), 0)
                           for g in grupos for x, a in enumerate(g) for b in g[x + 1:])
            if score_py == round(args.heur_score):
                fila["verif_afinidad"] = "ok"
            else:
                fila["verif_afinidad"] = f"DIFIERE (python={score_py})"
                print(f"  ADVERTENCIA: la solución de la heurística vale {score_py} en Python "
                      f"y {args.heur_score:.0f} en C++. Revisa λ, e(t) o saltos de línea (CRLF).")
        except KeyError as ex:
            fila["verif_afinidad"] = f"DAG no encontrado: {ex}"
    print(f"[{nombre}] DAGs: {n} | Dominios K: {K} | "
          f"pares con afinidad != 0: {len(aff):,} de {n*(n-1)//2:,}", flush=True)

    m = gp.Model("Agrupamiento_DAGs")
    m.Params.TimeLimit = args.time_limit
    if args.log:
        m.Params.LogFile = args.log
    if args.threads:
        m.Params.Threads = args.threads
    if args.mem_limit:
        m.Params.SoftMemLimit = args.mem_limit
    if args.silencioso:
        m.Params.LogToConsole = 0

    # Ruptura de simetría: el DAG i solo puede ir a dominios k <= i
    Y = m.addVars([(i, k) for i in range(n) for k in range(min(i, K - 1) + 1)],
                  vtype=GRB.BINARY, name="Y")
    U = m.addVars(K, vtype=GRB.BINARY, name="U")
    # X continua en [0,1]: con las restricciones de abajo es 0/1 en el óptimo
    X = m.addVars([(i, j, k) for (i, j) in aff for k in range(min(i, K - 1) + 1)],
                  lb=0, ub=1, name="X")

    # (2) Función objetivo: cada par no ordenado una sola vez
    m.setObjective(gp.quicksum(aff[i, j] * X[i, j, k] for (i, j, k) in X), GRB.MAXIMIZE)
    # (3) Unicidad
    m.addConstrs((Y.sum(i, "*") == 1 for i in range(n)), name="Unicidad")
    # (4) Capacidad
    m.addConstrs((gp.quicksum(dags[i]["w"] * Y[i, k] for i in range(k, n))
                  <= args.cmax * U[k] for k in range(K)), name="Capacidad")
    # (5) Todo dominio utilizado tiene al menos un DAG
    m.addConstrs((Y.sum("*", k) >= U[k] for k in range(K)), name="NoVacio")
    # (6)-(8) Linealización: solo el lado que el objetivo "empuja"
    for (i, j, k) in X:
        if aff[i, j] > 0:
            m.addConstr(X[i, j, k] <= Y[i, k])
            m.addConstr(X[i, j, k] <= Y[j, k])
        else:
            m.addConstr(X[i, j, k] >= Y[i, k] + Y[j, k] - 1)

    # MIP start opcional con la solución de la heurística
    if args.start:
        for k, miembros in enumerate(leer_solucion(args.start, dags)):
            for i in miembros:
                Y[i, k].Start = 1

    fila["grb_tiempo_construccion_s"] = fmt(time.time() - t0)
    m.optimize()

    estados = {GRB.OPTIMAL: "OPTIMO", GRB.TIME_LIMIT: "LIMITE_TIEMPO",
               GRB.INFEASIBLE: "INFACTIBLE", GRB.INTERRUPTED: "INTERRUMPIDO",
               GRB.MEM_LIMIT: "LIMITE_MEMORIA"}
    fila["grb_estado"] = estados.get(m.Status, str(m.Status))
    fila["grb_tiempo_solver_s"] = fmt(m.Runtime)

    print(f"\n[{nombre}] Estado: {fila['grb_estado']} | Tiempo solver: {m.Runtime:.1f} s")
    if m.SolCount > 0:
        fila["grb_obj"] = fmt(m.ObjVal, 0)
        fila["grb_cota"] = fmt(m.ObjBound, 0)
        fila["grb_gap_pct"] = fmt(100 * m.MIPGap)
        asign = {}
        for (i, k), v in Y.items():
            if v.X > 0.5:
                asign.setdefault(k, []).append(i)
        fila["grb_dominios_con_dags"] = len(asign)
        print(f"  Mejor solución: {m.ObjVal:.0f} | Cota: {m.ObjBound:.0f} | "
              f"Gap: {100 * m.MIPGap:.2f} % | Dominios con DAGs: {len(asign)}")
        if args.solucion_salida:
            with open(args.solucion_salida, "w", encoding="utf-8") as f:
                f.write("cluster_id,dag_name\n")
                for k in sorted(asign):
                    for i in asign[k]:
                        f.write(f"{k},{dags[i]['name']}\n")
    else:
        print("  Gurobi no encontró ninguna solución factible en el tiempo dado.")

    if args.heur_score is not None and m.Status != GRB.INFEASIBLE:
        try:
            cota = m.ObjBound
        except gp.GurobiError:
            cota = None
        if cota is not None and 0 < cota < GRB.INFINITY:
            fila.setdefault("grb_cota", fmt(cota, 0))
            brecha = 100 * (cota - args.heur_score) / cota
            fila["heur_brecha_vs_cota_pct"] = fmt(brecha)
            print(f"  Heurística ({args.heur_score:.0f}) a lo más {brecha:.2f} % bajo el óptimo")

    escribir_fila(args.resultados, fila)


def main():
    p = argparse.ArgumentParser(description="MIP exacto para el agrupamiento de DAGs")
    p.add_argument("--dags", default="Instances/dags_300.csv")
    p.add_argument("--pesos", default="Instances/important_tables.csv",
                   help="CSV de pesos e(t); si no existe se usa e(t)=0")
    p.add_argument("--lambdas", type=int, nargs=3, default=[18, 2, -16],
                   metavar=("L", "S", "C"), help="λ_L λ_S λ_C (los mismos del C++)")
    p.add_argument("--cmax", type=int, default=50)
    p.add_argument("--dominios", type=int, default=None,
                   help="K; por defecto, los dominios que abre el greedy")
    p.add_argument("--time-limit", type=float, default=3600)
    p.add_argument("--threads", type=int, default=0, help="0 = todos los núcleos")
    p.add_argument("--mem-limit", type=float, default=0, help="GB; 0 = sin límite")
    p.add_argument("--start", default=None, help="CSV de la heurística para MIP start")
    p.add_argument("--heur-score", type=float, default=None)
    p.add_argument("--heur-solucion", default=None,
                   help="CSV de la heurística (solo para verificar la afinidad)")
    p.add_argument("--heur-tiempo", type=float, default=None)
    p.add_argument("--heur-dominios-usados", type=int, default=None)
    p.add_argument("--heur-factible", default=None)
    p.add_argument("--resultados", default=None, help="CSV donde agregar una fila")
    p.add_argument("--solucion-salida", default=None, help="CSV con la asignación de Gurobi")
    p.add_argument("--log", default="gurobi.log")
    p.add_argument("--silencioso", action="store_true")
    args = p.parse_args()

    try:
        resolver(args)
    except (gp.GurobiError, ValueError, MemoryError) as ex:
        nombre = os.path.splitext(os.path.basename(args.dags))[0]
        msg = str(ex).replace(",", ";").replace("\n", " ")[:120]
        print(f"[{nombre}] ERROR: {msg}")
        try:
            n = len(cargar_dags(args.dags))
        except Exception:
            n = ""
        escribir_fila(args.resultados, {
            "instancia": nombre, "n_dags": n, "grb_estado": f"ERROR: {msg}",
            "heur_score": fmt(args.heur_score, 0), "heur_tiempo_s": fmt(args.heur_tiempo),
            "heur_dominios_con_dags": fmt(args.heur_dominios_usados),
            "heur_factible": fmt(args.heur_factible), "dominios_K": fmt(args.dominios),
            "mip_start": "si" if args.start else "no"})
        raise SystemExit(1)


if __name__ == "__main__":
    main()