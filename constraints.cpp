#include "types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <algorithm>

using namespace std;

extern int calculate_affinity(const DAG& dag1, const DAG& dag2);

// ==========================================================
// 1. Restricción de Capacidad del Clúster (Límite de I/O)
// ==========================================================
bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster){
    int new_dag_weight =
        new_dag.num_source_tables +
        new_dag.num_target_tables;

    return (cluster.current_capacity + new_dag_weight)
            <= max_tables_per_cluster;
}

// ==========================================================
// 2. Restricción de Unicidad
// Un DAG sólo puede pertenecer a un clúster.
// ==========================================================
bool check_uniqueness_constraint(const Solution& sol, int dag_index){
    int occurrences = 0;

    for (const auto& cluster : sol.clusters){
        for (int id : cluster.dags_indices){
            if (id == dag_index)
                occurrences++;
        }
    }

    return occurrences <= 1;
}

// ==========================================================
// 3. Restricción de Integridad del DAG
// Todo DAG debe poseer al menos una tabla source
// y una tabla target.
// ==========================================================
bool check_dag_integrity(const DAG& dag){
    return !dag.source_tables.empty() &&
           !dag.target_tables.empty();
}

// ==========================================================
// 4. Restricción de Conectividad
// Todo DAG del clúster debe compartir al menos
// una relación de afinidad con otro DAG.
// ==========================================================
bool check_connectivity_constraint(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    if (cluster.dags_indices.empty())
        return true;

    for (int idx : cluster.dags_indices){
        if (calculate_affinity(candidate, all_dags[idx]) > 0)
            return true;
    }

    return false;
}

// ==========================================================
// 5. Conflictos de Escritura
// No bloquea el movimiento.
// Sólo devuelve la penalización.
// ==========================================================
int count_target_conflicts(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    unordered_map<string,int> existing_targets;

    for (int idx : cluster.dags_indices){
        for (const auto& t : all_dags[idx].target_tables)
            existing_targets[t]++;
    }

    int conflicts = 0;

    for (const auto& t : candidate.target_tables){
        if (existing_targets.count(t))
            conflicts++;
    }

    return conflicts;
}

// ==========================================================
// Movimiento Factible
// ==========================================================
bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const vector<DAG>& all_dags, int max_capacity){
    if (!check_dag_integrity(dag_to_move))
        return false;

    if (!check_capacity_constraint(target_cluster, dag_to_move, max_capacity))
        return false;

    if (!check_connectivity_constraint(target_cluster, dag_to_move, all_dags))
        return false;

    return true;
}