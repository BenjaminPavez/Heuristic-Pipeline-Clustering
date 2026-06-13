#include "types.h"
#include <unordered_set>
#include <string>

// Calcula cuántas tablas únicas comparten dos DAGs
int calculate_affinity(const DAG& dag1, const DAG& dag2) {
    int shared_tables = 0;
    
    // 1. Consolidar todas las tablas (origen y destino) del dag1 en un set hash
    std::unordered_set<std::string> dag1_tables;
    for (const auto& table : dag1.source_tables) {
        dag1_tables.insert(table);
    }
    for (const auto& table : dag1.target_tables) {
        dag1_tables.insert(table);
    }

    // 2. Consolidar las tablas del dag2 en otro set temporal para evitar duplicados 
    // (en caso de que un DAG lea y escriba en la misma tabla)
    std::unordered_set<std::string> dag2_tables;
    for (const auto& table : dag2.source_tables) {
        dag2_tables.insert(table);
    }
    for (const auto& table : dag2.target_tables) {
        dag2_tables.insert(table);
    }

    // 3. Contar la intersección: cuántas tablas del dag2 existen en el dag1
    for (const auto& table : dag2_tables) {
        // find() en un unordered_set es O(1)
        if (dag1_tables.find(table) != dag1_tables.end()) {
            shared_tables++;
        }
    }

    return shared_tables;
}

// Calcula el fitness total de una solución completa
int evaluate_solution(const Solution& sol, const std::vector<DAG>& all_dags) {
    int total_fitness = 0;
    
    // Por cada clúster (Dominio de Datos), sumar la afinidad cruzada de todos los DAGs que contiene
    for (const auto& cluster : sol.clusters) {
        // Comparamos todos los pares de DAGs posibles dentro del clúster
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i) {
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j) {
                total_fitness += calculate_affinity(
                    all_dags[cluster.dags_indices[i]], 
                    all_dags[cluster.dags_indices[j]]
                );
            }
        }
    }
    return total_fitness;
}