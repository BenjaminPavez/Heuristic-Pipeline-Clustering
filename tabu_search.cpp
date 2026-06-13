#include "tabu_search.h"
#include <vector>
#include <iostream>

// Declaraciones de funciones externas
extern int evaluate_solution(const Solution& sol, const std::vector<DAG>& all_dags);
extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_capacity);

// Helper para obtener el peso de un DAG (I/O)
int get_dag_weight(const DAG& dag) {
    return dag.num_source_tables + dag.num_target_tables;
}

Solution run_tabu_search(const Solution& initial_sol, const std::vector<DAG>& dags, int max_capacity, int max_iterations, int tabu_tenure) {
    Solution current_sol = initial_sol;
    current_sol.fitness_score = evaluate_solution(current_sol, dags);
    
    Solution best_global_sol = current_sol;
    
    // Matriz Tabú: tabu_matrix[dag_id][cluster_id] guarda la iteración en la que expira la restricción
    std::vector<std::vector<int>> tabu_matrix(dags.size(), std::vector<int>(current_sol.clusters.size(), 0));

    for (int iter = 1; iter <= max_iterations; ++iter) {
        Solution best_neighbor_sol;
        int best_neighbor_fitness = -1;
        
        int best_move_dag_id = -1;
        int best_move_from_idx = -1;
        int best_move_to_idx = -1;

        // Exploración del Vecindario: Intentar mover cada DAG a otro clúster
        for (size_t from_idx = 0; from_idx < current_sol.clusters.size(); ++from_idx) {
            for (size_t d_idx = 0; d_idx < current_sol.clusters[from_idx].dags_indices.size(); ++d_idx) {
                
                int dag_id = current_sol.clusters[from_idx].dags_indices[d_idx];
                int dag_weight = get_dag_weight(dags[dag_id]);

                for (size_t to_idx = 0; to_idx < current_sol.clusters.size(); ++to_idx) {
                    if (from_idx == to_idx) continue; // No mover al mismo clúster

                    // 1. Verificar Restricción Dura (Capacidad del servidor)
                    if (!check_capacity_constraint(current_sol.clusters[to_idx], dags[dag_id], max_capacity)) {
                        continue; // Movimiento infactible por I/O
                    }

                    // 2. Crear solución vecina (Movimiento)
                    Solution neighbor_sol = current_sol;
                    
                    // Remover del clúster origen
                    neighbor_sol.clusters[from_idx].dags_indices.erase(neighbor_sol.clusters[from_idx].dags_indices.begin() + d_idx);
                    neighbor_sol.clusters[from_idx].current_capacity -= dag_weight;
                    
                    // Agregar al clúster destino
                    neighbor_sol.clusters[to_idx].dags_indices.push_back(dag_id);
                    neighbor_sol.clusters[to_idx].current_capacity += dag_weight;

                    // Evaluar el fitness del vecino
                    int neighbor_fitness = evaluate_solution(neighbor_sol, dags);

                    // 3. Condición Tabú y Criterio de Aspiración
                    bool is_tabu = tabu_matrix[dag_id][to_idx] >= iter;
                    bool aspiration = neighbor_fitness > best_global_sol.fitness_score; // Si es el mejor de la historia, ignorar Tabú

                    if (!is_tabu || aspiration) {
                        if (neighbor_fitness > best_neighbor_fitness) {
                            best_neighbor_fitness = neighbor_fitness;
                            best_neighbor_sol = neighbor_sol;
                            best_move_dag_id = dag_id;
                            best_move_from_idx = from_idx;
                            best_move_to_idx = to_idx;
                        }
                    }
                }
            }
        }

        // Si no se encontró ningún vecino válido, terminar la búsqueda
        if (best_move_dag_id == -1) {
            break; 
        }

        // 4. Aplicar el mejor movimiento encontrado
        current_sol = best_neighbor_sol;
        current_sol.fitness_score = best_neighbor_fitness;

        // 5. Actualizar la Lista Tabú (Evitar regresar el DAG al clúster del que acaba de salir)
        tabu_matrix[best_move_dag_id][best_move_from_idx] = iter + tabu_tenure;

        // 6. Actualizar el mejor global
        if (current_sol.fitness_score > best_global_sol.fitness_score) {
            best_global_sol = current_sol;
        }
    }

    return best_global_sol;
}