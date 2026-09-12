import os
import glob
import csv
import gurobipy as gp
from gurobipy import GRB

# ==========================================
# 1. PARÁMETROS DEL MODELO (CONFIGURACIÓN)
# ==========================================
INPUT_DIR = 'Instances'
OUTPUT_SOLVED_DIR = 'Solved/Gurobi'
OUTPUT_LOGS_DIR = 'Logs/Gurobi'

# Pesos utilizados en el algoritmo C++
LAMBDA_S = 1         # WEIGHT_SHARED_TABLE
LAMBDA_L = 3         # WEIGHT_LINEAGE_RELATION
LAMBDA_C = -4        # WEIGHT_WRITE_CONFLICT

C_MAX = 20           # Capacidad máxima de I/O por Dominio
TIEMPO_LIMITE = 3600 # Límite de tiempo en segundos (1 hora por instancia)

# ==========================================
# 2. CONFIGURACIÓN DE DIRECTORIOS
# ==========================================
os.makedirs(OUTPUT_SOLVED_DIR, exist_ok=True)
os.makedirs(OUTPUT_LOGS_DIR, exist_ok=True)

def procesar_instancia(filepath):
    basename = os.path.basename(filepath).replace('.csv', '')
    log_path = os.path.join(OUTPUT_LOGS_DIR, f"{basename}.txt")
    sol_path = os.path.join(OUTPUT_SOLVED_DIR, f"{basename}_solved.csv")
    
    print(f"\n[{basename}] Procesando instancia...")

    # 3. LECTURA DE DATOS
    dags = []
    S = {} 
    W = {} 
    
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter=';')
        for row in reader:
            dag = row['DAG']
            dags.append(dag)
            
            tablas_w = row['TablasAlimenta'].strip()
            tablas_s = row['TablasAlimentan'].strip()
            
            W[dag] = set(tablas_w.split(',')) if tablas_w else set()
            S[dag] = set(tablas_s.split(',')) if tablas_s else set()

    num_dags = len(dags)
    num_dominios = num_dags 
    w = {i: len(S[dags[i]]) + len(W[dags[i]]) for i in range(num_dags)}

    # 4. CÁLCULO DE LA MATRIZ DE AFINIDAD
    aff = {}
    for i in range(num_dags):
        for j in range(i + 1, num_dags): 
            d_i = dags[i]
            d_j = dags[j]
            
            C_ij = W[d_i].intersection(W[d_j])
            
            L_ij_bruto = (W[d_i].intersection(S[d_j])).union(W[d_j].intersection(S[d_i]))
            L_ij = L_ij_bruto - C_ij
            
            S_ij_bruto = S[d_i].intersection(S[d_j])
            S_ij = S_ij_bruto - L_ij - C_ij
            
            afinidad = (LAMBDA_L * len(L_ij)) + (LAMBDA_S * len(S_ij)) + (LAMBDA_C * len(C_ij))
            aff[i, j] = afinidad

    # 5. CONSTRUCCIÓN DEL MODELO EN GUROBI
    env = gp.Env(empty=True)
    env.setParam('LogToConsole', 0) # Silenciar consola para no saturar la terminal
    env.start()
    
    m = gp.Model(f"Modelo_{basename}", env=env)
    m.setParam('TimeLimit', TIEMPO_LIMITE)
    m.setParam('LogFile', log_path) # Gurobi guardará el log detallado aquí directamente

    Y = m.addVars(num_dags, num_dominios, vtype=GRB.BINARY, name="Y")
    U = m.addVars(num_dominios, vtype=GRB.BINARY, name="U")
    X = m.addVars(num_dags, num_dags, num_dominios, vtype=GRB.BINARY, name="X")

    obj = gp.quicksum(aff[i,j] * X[i,j,k] 
                      for i in range(num_dags) 
                      for j in range(i + 1, num_dags) 
                      for k in range(num_dominios))
    m.setObjective(obj, GRB.MAXIMIZE)

    for i in range(num_dags):
        m.addConstr(gp.quicksum(Y[i,k] for k in range(num_dominios)) == 1)

    for k in range(num_dominios):
        m.addConstr(gp.quicksum(w[i] * Y[i,k] for i in range(num_dags)) <= C_MAX * U[k])
        m.addConstr(gp.quicksum(Y[i,k] for i in range(num_dags)) >= U[k])

    for i in range(num_dags):
        for j in range(i + 1, num_dags):
            for k in range(num_dominios):
                m.addConstr(X[i,j,k] <= Y[i,k])
                m.addConstr(X[i,j,k] <= Y[j,k])
                m.addConstr(X[i,j,k] >= Y[i,k] + Y[j,k] - 1)

    for k in range(num_dominios - 1):
        m.addConstr(U[k] >= U[k+1])

    # 6. OPTIMIZACIÓN Y ESCRITURA DE RESULTADOS
    m.optimize()

    if m.SolCount > 0:
        with open(sol_path, 'w', newline='', encoding='utf-8') as f_out:
            writer = csv.writer(f_out, delimiter=';')
            writer.writerow(['cluster_id', 'dag_name'])
            
            cluster_id_real = 0
            for k in range(num_dominios):
                if U[k].X > 0.5: 
                    for i in range(num_dags):
                        if Y[i,k].X > 0.5:
                            writer.writerow([cluster_id_real, dags[i]])
                    cluster_id_real += 1
                    
        estado = "ÓPTIMO" if m.status == GRB.OPTIMAL else "LÍMITE DE TIEMPO"
        print(f"[{basename}] Completado: {estado} | Score: {m.objVal} | Tiempo: {m.Runtime:.2f}s")
    else:
        print(f"[{basename}] Falló: No se encontró solución factible (Infactible o Falta de Memoria).")

# ==========================================
# EJECUCIÓN POR LOTES
# ==========================================
archivos_instancias = glob.glob(os.path.join(INPUT_DIR, 'dags_*.csv'))
archivos_instancias.sort()

print(f"Se encontraron {len(archivos_instancias)} instancias para procesar.")

for archivo in archivos_instancias:
    procesar_instancia(archivo)