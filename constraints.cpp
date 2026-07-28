#include "types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <algorithm>

using namespace std;

extern int calculate_affinity(const DAG& dag1, const DAG& dag2);

// ==========================================================
// 1. Restricción de Capacidad (DURA)
// Bloquea el movimiento si el clúster supera C_max de I/O.
// ==========================================================
bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster){
    int new_dag_weight = new_dag.num_source_tables + new_dag.num_target_tables;
    return (cluster.current_capacity + new_dag_weight) <= max_tables_per_cluster;
}

// ==========================================================
// 2. Restricción de Unicidad (DURA, validación de solución)
// Verifica que un DAG no esté asignado a más de un clúster.
// No se llama durante la búsqueda porque el Tabu Search
// garantiza unicidad por construcción. Se usa en
// is_feasible_solution() para validar soluciones completas.
// ==========================================================
bool check_uniqueness_constraint(const Solution& sol, int dag_index){
    int occurrences = 0;
    for (const auto& cluster : sol.clusters)
        for (int id : cluster.dags_indices)
            if (id == dag_index) occurrences++;
    return occurrences <= 1;
}

// ==========================================================
// 3. Integridad del DAG (DURA, validación de datos)
// Todo DAG debe tener al menos una tabla source y una target.
// Se llama en utils.cpp al cargar los datos, no durante
// la búsqueda, ya que un DAG no pierde sus tablas al moverse.
// ==========================================================
bool check_dag_integrity(const DAG& dag){
    return !dag.source_tables.empty() && !dag.target_tables.empty();
}

// ==========================================================
// 4. Conectividad (INFORMATIVA, no bloquea movimientos)
// Indica si un DAG candidato tiene afinidad > 0 con algún
// DAG ya en el clúster. Se usa en reporting para medir
// cohesión, pero NO en is_feasible_move porque bloquearía
// movimientos válidos que mejoran el fitness global.
// ==========================================================
bool check_connectivity_constraint(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    if (cluster.dags_indices.empty()) return true;
    for (int idx : cluster.dags_indices)
        if (calculate_affinity(candidate, all_dags[idx]) > 0)
            return true;
    return false;
}

// ==========================================================
// 5. Conflictos de Escritura (BLANDA, no bloquea movimientos)
// Cuenta cuántas tablas target del candidato ya son escritas
// por otro DAG en el clúster. Se usa en reporting.
// Durante la búsqueda, los conflictos ya están penalizados
// automáticamente por WEIGHT_WRITE_CONFLICT en calculate_affinity.
// ==========================================================
int count_target_conflicts(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    unordered_map<string,int> existing_targets;
    for (int idx : cluster.dags_indices)
        for (const auto& t : all_dags[idx].target_tables)
            existing_targets[t]++;
    int conflicts = 0;
    for (const auto& t : candidate.target_tables)
        if (existing_targets.count(t)) conflicts++;
    return conflicts;
}

// ==========================================================
// Movimiento Factible
// Solo aplica restricciones DURAS que deben verificarse
// en cada movimiento durante la búsqueda:
//   - Capacidad I/O del servidor
// La integridad se valida al cargar datos (utils.cpp).
// La unicidad la garantiza el algoritmo por construcción.
// La conectividad y conflictos se manejan en el fitness.
// ==========================================================
bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move,
                      const vector<DAG>& all_dags [[maybe_unused]], int max_capacity){
    return check_capacity_constraint(target_cluster, dag_to_move, max_capacity);
}