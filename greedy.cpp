#include "greedy.h"
#include "types.h"
#include <vector>
#include <iostream>

// Declaraciones externas de las funciones que programamos en evaluation.cpp y constraints.cpp
extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_capacity);
extern int calculate_affinity(const DAG& dag1, const DAG& dag2);

// Calcula la afinidad acumulada de un DAG candidato con todos los DAGs que ya estan en el cluster
int calculate_cluster_affinity(const Cluster& cluster, const DAG& candidate_dag, const std::vector<DAG>& all_dags) {
    int total_affinity = 0;
    for (int dag_idx : cluster.dags_indices) {
        total_affinity += calculate_affinity(all_dags[dag_idx], candidate_dag);
    }
    return total_affinity;
}

// Genera la solucion inicial factible
Solution run_greedy(const std::vector<DAG>& dags, int max_capacity) {
    Solution sol;
    std::vector<bool> assigned(dags.size(), false);
    int unassigned_count = dags.size();
    int cluster_id_counter = 0;

    // Mientras existan DAGs sin agrupar
    while (unassigned_count > 0) {
        Cluster current_cluster;
        current_cluster.id = cluster_id_counter++;
        current_cluster.current_capacity = 0;

        bool added_to_cluster = true;

        // Intentar llenar el cluster actual hasta que no quepan mas DAGs
        while (added_to_cluster && unassigned_count > 0) {
            added_to_cluster = false;
            int best_dag_idx = -1;
            int best_affinity = -1;

            // Explorar todos los DAGs para encontrar el mejor candidato
            for (size_t i = 0; i < dags.size(); ++i) {
                if (!assigned[i]) {
                    
                    // Si el cluster esta vacio, priorizamos el DAG "mas pesado" como semilla inicial
                    int current_affinity = (current_cluster.dags_indices.empty()) ? 
                                           (dags[i].num_source_tables + dags[i].num_target_tables) : 
                                           calculate_cluster_affinity(current_cluster, dags[i], dags);

                    // Si la afinidad es la mejor hasta ahora, verificamos si cabe en el servidor
                    if (current_affinity > best_affinity) {
                        if (check_capacity_constraint(current_cluster, dags[i], max_capacity)) {
                            best_affinity = current_affinity;
                            best_dag_idx = i;
                        }
                    }
                }
            }

            // Si encontramos un DAG valido que no rompa las restricciones, lo empaquetamos
            if (best_dag_idx != -1) {
                current_cluster.dags_indices.push_back(best_dag_idx);
                // Actualizamos la ocupacion de la caja
                current_cluster.current_capacity += (dags[best_dag_idx].num_source_tables + dags[best_dag_idx].num_target_tables);
                assigned[best_dag_idx] = true;
                unassigned_count--;
                added_to_cluster = true; // Confirmamos que el cluster sigue activo
            }
        }

        // Una vez que el cluster actual no puede recibir mas DAGs (por capacidad o falta de afinidad), lo guardamos
        sol.clusters.push_back(current_cluster);
    }

    return sol;
}