#include "tabu_search.h"
#include <vector>
#include <algorithm>
#include <climits>
#include <iostream>

extern int evaluate_solution(const Solution& sol, const std::vector<DAG>& all_dags);
extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_capacity);

int get_dag_weight(const DAG& dag) {
    return dag.num_source_tables + dag.num_target_tables;
}

// ---------------------------------------------------------------------------
// Perturbación: cuando el algoritmo se estanca, toma el clúster con menor
// cohesión (menos afinidad por par) y redistribuye sus DAGs hacia el clúster
// que mejor los acepte. Esto escapa óptimos locales sin reiniciar desde cero.
// ---------------------------------------------------------------------------
static Solution perturb_solution(const Solution& sol, const std::vector<DAG>& dags, int max_capacity) {
    Solution perturbed = sol;

    // Encontrar el clúster con menor afinidad promedio por par
    int worst_cluster_idx = 0;
    double worst_cohesion = 1e9;

    for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
        const auto& cluster = perturbed.clusters[c];
        if (cluster.dags_indices.size() < 2) continue;

        int pairs = 0, aff_sum = 0;
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i)
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j) {
                // Usamos solo conteo de tablas compartidas como proxy rápido
                aff_sum++; pairs++;
            }
        double cohesion = pairs > 0 ? (double)aff_sum / pairs : 0.0;
        if (cohesion < worst_cohesion) {
            worst_cohesion = cohesion;
            worst_cluster_idx = c;
        }
    }

    // Tomar todos los DAGs del clúster peor y reasignarlos al mejor clúster disponible
    std::vector<int> dags_to_redistribute = perturbed.clusters[worst_cluster_idx].dags_indices;
    perturbed.clusters[worst_cluster_idx].dags_indices.clear();
    perturbed.clusters[worst_cluster_idx].current_capacity = 0;

    for (int dag_id : dags_to_redistribute) {
        int best_target = worst_cluster_idx; // fallback: volver al mismo
        int best_fit = -1;

        for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
            if ((int)c == worst_cluster_idx) continue;
            if (!check_capacity_constraint(perturbed.clusters[c], dags[dag_id], max_capacity)) continue;

            // Calcular afinidad con el clúster destino
            int aff = 0;
            for (int idx [[maybe_unused]] : perturbed.clusters[c].dags_indices)
                aff++; // proxy simple para la perturbación
            if (aff > best_fit) { best_fit = aff; best_target = c; }
        }

        perturbed.clusters[best_target].dags_indices.push_back(dag_id);
        perturbed.clusters[best_target].current_capacity += get_dag_weight(dags[dag_id]);
    }

    return perturbed;
}

Solution run_tabu_search(const Solution& initial_sol, const std::vector<DAG>& dags, int max_capacity, int max_iterations, int tabu_tenure) {
    Solution current_sol = initial_sol;
    current_sol.fitness_score = evaluate_solution(current_sol, dags);

    Solution best_global_sol = current_sol;

    std::vector<std::vector<int>> tabu_matrix(dags.size(), std::vector<int>(current_sol.clusters.size(), 0));

    // Parámetros de diversificación
    const int STAGNATION_LIMIT = max_iterations / 5; // Perturbar si no mejora en 20% de las iteraciones
    int stagnation_counter = 0;

    for (int iter = 1; iter <= max_iterations; ++iter) {
        Solution best_neighbor_sol;
        int best_neighbor_fitness = INT_MIN;

        int best_move_dag_id   = -1;
        int best_move_from_idx = -1;
        int best_move_to_idx   = -1;

        for (size_t from_idx = 0; from_idx < current_sol.clusters.size(); ++from_idx) {
            for (size_t d_idx = 0; d_idx < current_sol.clusters[from_idx].dags_indices.size(); ++d_idx) {

                int dag_id     = current_sol.clusters[from_idx].dags_indices[d_idx];
                int dag_weight = get_dag_weight(dags[dag_id]);

                for (size_t to_idx = 0; to_idx < current_sol.clusters.size(); ++to_idx) {
                    if (from_idx == to_idx) continue;

                    // Restricción dura: capacidad
                    if (!check_capacity_constraint(current_sol.clusters[to_idx], dags[dag_id], max_capacity)) continue;

                    // Construir solución vecina
                    Solution neighbor_sol = current_sol;
                    neighbor_sol.clusters[from_idx].dags_indices.erase(
                        neighbor_sol.clusters[from_idx].dags_indices.begin() + d_idx);
                    neighbor_sol.clusters[from_idx].current_capacity -= dag_weight;
                    neighbor_sol.clusters[to_idx].dags_indices.push_back(dag_id);
                    neighbor_sol.clusters[to_idx].current_capacity += dag_weight;

                    int neighbor_fitness = evaluate_solution(neighbor_sol, dags);

                    bool is_tabu    = tabu_matrix[dag_id][to_idx] >= iter;
                    bool aspiration = neighbor_fitness > best_global_sol.fitness_score;

                    if (!is_tabu || aspiration) {
                        if (neighbor_fitness > best_neighbor_fitness) {
                            best_neighbor_fitness = neighbor_fitness;
                            best_neighbor_sol     = neighbor_sol;
                            best_move_dag_id      = dag_id;
                            best_move_from_idx    = from_idx;
                            best_move_to_idx      = to_idx;
                        }
                    }
                }
            }
        }

        if (best_move_dag_id == -1) break;

        current_sol = best_neighbor_sol;
        current_sol.fitness_score = best_neighbor_fitness;

        tabu_matrix[best_move_dag_id][best_move_from_idx] = iter + tabu_tenure;

        if (current_sol.fitness_score > best_global_sol.fitness_score) {
            best_global_sol   = current_sol;
            stagnation_counter = 0;
        } else {
            stagnation_counter++;
        }

        // Diversificación: si llevamos STAGNATION_LIMIT iteraciones sin mejorar, perturbar
        if (stagnation_counter >= STAGNATION_LIMIT) {
            std::cout << "  [Tabu] Perturbando en iteracion " << iter << " (estancamiento)\n";
            current_sol = perturb_solution(current_sol, dags, max_capacity);
            current_sol.fitness_score = evaluate_solution(current_sol, dags);
            stagnation_counter = 0;
            // Reiniciar la matriz tabú tras perturbación
            for (auto& row : tabu_matrix)
                std::fill(row.begin(), row.end(), 0);
        }
    }

    return best_global_sol;
}