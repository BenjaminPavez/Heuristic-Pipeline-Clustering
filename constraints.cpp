#include "types.h"
#include <vector>
#include <unordered_map>
#include <iostream>

// 1. Restricción de Capacidad del Clúster (Límite de I/O)
bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster) {
    int new_dag_weight = new_dag.num_source_tables + new_dag.num_target_tables;
    if (cluster.current_capacity + new_dag_weight > max_tables_per_cluster) {
        return false;
    }
    return true;
}

// 2. Restricción de Unicidad
bool check_uniqueness_constraint(const Solution& sol, int dag_index) {
    int occurrences = 0;
    for (const auto& cluster : sol.clusters) {
        for (int id : cluster.dags_indices) {
            if (id == dag_index) occurrences++;
        }
    }
    return occurrences <= 1;
}

// 3. Contar conflictos de escritura al agregar un DAG a un clúster
// Retorna cuántas tablas target del candidato ya son escritas por otro DAG en el clúster
// No bloquea el movimiento; el valor se usa como penalización en el fitness
int count_target_conflicts(const Cluster& cluster, const DAG& candidate, const std::vector<DAG>& all_dags) {
    // Construir set de tablas target ya presentes en el clúster
    std::unordered_map<std::string, int> existing_targets;
    for (int idx : cluster.dags_indices) {
        for (const auto& t : all_dags[idx].target_tables) {
            existing_targets[t]++;
        }
    }
    int conflicts = 0;
    for (const auto& t : candidate.target_tables) {
        if (existing_targets.count(t)) conflicts++;
    }
    return conflicts;
}

// Función orquestadora: valida capacidad (restricción dura)
// Los conflictos de escritura se penalizan en fitness, no bloquean el movimiento
bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const Solution& current_sol, int max_capacity) {
    if (!check_capacity_constraint(target_cluster, dag_to_move, max_capacity)) {
        return false;
    }
    return true;
}