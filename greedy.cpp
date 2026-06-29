#include "greedy.h"
#include "types.h"
#include <vector>
#include <climits>
#include <iostream>

extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_capacity);
extern int calculate_affinity(const DAG& dag1, const DAG& dag2);

// Calcula la afinidad acumulada de un DAG candidato con todos los DAGs ya en el clúster
int calculate_cluster_affinity(const Cluster& cluster, const DAG& candidate_dag, const std::vector<DAG>& all_dags) {
    int total_affinity = 0;
    for (int dag_idx : cluster.dags_indices) {
        total_affinity += calculate_affinity(all_dags[dag_idx], candidate_dag);
    }
    return total_affinity;
}

Solution run_greedy(const std::vector<DAG>& dags, int max_capacity) {
    Solution sol;
    std::vector<bool> assigned(dags.size(), false);
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
            // BUG FIX: usar INT_MIN en vez de -1 para que afinidad 0 sea candidato válido
            int best_affinity = INT_MIN;

            for (size_t i = 0; i < dags.size(); ++i) {
                if (assigned[i]) continue;

                // Si el clúster está vacío, usar el peso del DAG como criterio semilla
                // para arrancar con el DAG más "pesado" (más tablas = más potencial de afinidad)
                int current_affinity = current_cluster.dags_indices.empty()
                    ? (dags[i].num_source_tables + dags[i].num_target_tables)
                    : calculate_cluster_affinity(current_cluster, dags[i], dags);

                if (current_affinity > best_affinity) {
                    if (check_capacity_constraint(current_cluster, dags[i], max_capacity)) {
                        best_affinity = current_affinity;
                        best_dag_idx = i;
                    }
                }
            }

            // Si encontramos candidato válido, lo agregamos
            if (best_dag_idx != -1) {
                int dag_weight = dags[best_dag_idx].num_source_tables + dags[best_dag_idx].num_target_tables;
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