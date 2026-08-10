#include "greedy.h"
#include "types.h"
#include <vector>
#include <climits>
#include <algorithm>
#include <iostream>

using namespace std;

extern bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const vector<DAG>& all_dags, int max_capacity);
extern int calculate_affinity(const DAG& dag1, const DAG& dag2, const vector<TableImportance>& optional_weights);

// ---------------------------------------------------------------------------
// sum_extra_weights
// Suma los pesos extra de todas las tablas (source y target) de un DAG
// que aparezcan en optional_weights. Se usa como bonus en la semilla del greedy
// para priorizar DAGs que contienen tablas conocidas como importantes.
// ---------------------------------------------------------------------------
int sum_extra_weights(const DAG& dag, const vector<TableImportance>& optional_weights) {
    int total = 0;

    for (const auto& t : dag.source_tables) {
        auto it = find_if(optional_weights.begin(), optional_weights.end(), FindByName(t));
        if (it != optional_weights.end())
            total += it->extraWeight;
    }

    for (const auto& t : dag.target_tables) {
        auto it = find_if(optional_weights.begin(), optional_weights.end(), FindByName(t));
        if (it != optional_weights.end())
            total += it->extraWeight;
    }

    return total;
}

// ---------------------------------------------------------------------------
// calculate_cluster_affinity
// Afinidad acumulada de un DAG candidato con todos los DAGs ya en el clúster.
// ---------------------------------------------------------------------------
int calculate_cluster_affinity(const Cluster& cluster, const DAG& candidate_dag,
                                const vector<DAG>& all_dags,
                                const vector<TableImportance>& optional_weights) {
    int total_affinity = 0;
    for (int dag_idx : cluster.dags_indices)
        total_affinity += calculate_affinity(all_dags[dag_idx], candidate_dag, optional_weights);
    return total_affinity;
}

// ---------------------------------------------------------------------------
// run_greedy
// Construye la solución inicial. Cuando el clúster está vacío, la semilla
// es el peso I/O del DAG más el bonus de tablas importantes (sum_extra_weights),
// priorizando DAGs con tablas críticas como punto de partida del dominio.
// ---------------------------------------------------------------------------
Solution run_greedy(const vector<DAG>& dags,
                    const vector<TableImportance>& optional_weights,
                    int max_capacity) {
    Solution sol;
    vector<bool> assigned(dags.size(), false);
    int unassigned_count = dags.size();
    int cluster_id_counter = 0;

    while (unassigned_count > 0) {
        Cluster current_cluster;
        current_cluster.id = cluster_id_counter++;
        current_cluster.current_capacity = 0;
        current_cluster.write_conflicts = 0;

        bool added_to_cluster = true;

        while (added_to_cluster && unassigned_count > 0) {
            added_to_cluster = false;
            int best_dag_idx = -1;
            int best_affinity = INT_MIN;

            for (size_t i = 0; i < dags.size(); ++i) {
                if (assigned[i]) continue;

                int current_affinity;
                if (current_cluster.dags_indices.empty()) {
                    // Semilla: peso I/O + bonus de tablas importantes
                    int io_weight = dags[i].num_source_tables + dags[i].num_target_tables;
                    int bonus     = sum_extra_weights(dags[i], optional_weights);
                    current_affinity = io_weight + bonus;
                } else {
                    current_affinity = calculate_cluster_affinity(
                        current_cluster, dags[i], dags, optional_weights);
                }

                if (current_affinity > best_affinity) {
                    if (is_feasible_move(current_cluster, dags[i], dags, max_capacity)) {
                        best_affinity = current_affinity;
                        best_dag_idx  = i;
                    }
                }
            }

            if (best_dag_idx != -1) {
                int dag_weight = dags[best_dag_idx].num_source_tables
                               + dags[best_dag_idx].num_target_tables;
                current_cluster.dags_indices.push_back(best_dag_idx);
                current_cluster.current_capacity += dag_weight;
                assigned[best_dag_idx] = true;
                unassigned_count--;
                added_to_cluster = true;
            }
        }

        sol.clusters.push_back(current_cluster);
    }

    return sol;
}