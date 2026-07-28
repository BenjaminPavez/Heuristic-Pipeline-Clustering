#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------------
// calculate_affinity: Calcula la afinidad entre dos DAGs distinguiendo tres casos:
//
//  +WEIGHT_LINEAGE_RELATION  por cada relación productor→consumidor
//    (dag1 escribe una tabla que dag2 lee, o viceversa)
//
//  +WEIGHT_SHARED_TABLE      por cada tabla compartida genérica
//    (ambos la leen, o comparten sin relación directa de linaje)
//
//  +WEIGHT_WRITE_CONFLICT    (negativo) por cada tabla que AMBOS escriben
//    (conflicto de escritura simultánea en el mismo dominio)
// ---------------------------------------------------------------------------
int calculate_affinity(const DAG& dag1, const DAG& dag2) {
    int score = 0;

    // Sets de targets para detectar conflictos y linaje
    unordered_set<string> targets1(dag1.target_tables.begin(), dag1.target_tables.end());
    unordered_set<string> targets2(dag2.target_tables.begin(), dag2.target_tables.end());
    unordered_set<string> sources1(dag1.source_tables.begin(), dag1.source_tables.end());
    unordered_set<string> sources2(dag2.source_tables.begin(), dag2.source_tables.end());

    // Tablas ya contabilizadas para evitar doble conteo
    unordered_set<string> counted;

    // 1. Conflictos de escritura: ambos escriben la misma tabla (peor caso)
    for (const auto& t : targets1) {
        if (targets2.count(t)) {
            score += WEIGHT_WRITE_CONFLICT;
            counted.insert(t);
        }
    }

    // 2. Relación de linaje: dag1 escribe lo que dag2 lee
    for (const auto& t : targets1) {
        if (!counted.count(t) && sources2.count(t)) {
            score += WEIGHT_LINEAGE_RELATION;
            counted.insert(t);
        }
    }

    // 3. Relación de linaje inversa: dag2 escribe lo que dag1 lee
    for (const auto& t : targets2) {
        if (!counted.count(t) && sources1.count(t)) {
            score += WEIGHT_LINEAGE_RELATION;
            counted.insert(t);
        }
    }

    // 4. Tablas genéricas compartidas: ambos leen la misma tabla fuente
    for (const auto& t : sources1) {
        if (!counted.count(t) && sources2.count(t)) {
            score += WEIGHT_SHARED_TABLE;
            counted.insert(t);
        }
    }

    return score;
}

// ---------------------------------------------------------------------------
// evaluate_solution: Suma la afinidad cruzada de todos los pares dentro
// de cada clúster. También actualiza write_conflicts por clúster.
// ---------------------------------------------------------------------------
int evaluate_solution(Solution& sol, const vector<DAG>& all_dags) {
    int total_fitness = 0;

    for (auto& cluster : sol.clusters) {
        cluster.write_conflicts = 0;

        // Detectar conflictos de escritura dentro del clúster
        // Mapa: tabla_target -> lista de DAGs que escriben en ella
        unordered_map<string, int> target_writers;
        for (int idx : cluster.dags_indices) {
            for (const auto& t : all_dags[idx].target_tables) {
                target_writers[t]++;
            }
        }
        for (const auto& [table, count] : target_writers) {
            if (count > 1) cluster.write_conflicts += (count - 1);
        }

        // Sumar afinidad de todos los pares
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

// Sobrecarga const para uso interno del Tabu Search (sin actualizar write_conflicts)
int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags) {
    int total_fitness = 0;
    for (const auto& cluster : sol.clusters) {
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
// ---------------------------------------------------------------------------
// Declaraciones de funciones externas de constraints.cpp
// ---------------------------------------------------------------------------
extern bool check_uniqueness_constraint(const Solution& sol, int dag_index);
extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster);

// ---------------------------------------------------------------------------
// is_feasible_solution
// Valida que una solución completa cumpla todas las restricciones duras,
// llamando explícitamente a las funciones definidas en constraints.cpp.
// Se llama en main.cpp tras el greedy y tras el tabu search como
// verificación de correctitud (sanity check).
// ---------------------------------------------------------------------------
bool is_feasible_solution(const Solution& sol, const vector<DAG>& dags, int max_capacity) {
    // R1: Unicidad — cada DAG debe aparecer exactamente una vez
    for (size_t i = 0; i < dags.size(); ++i) {
        if (!check_uniqueness_constraint(sol, (int)i)) {
            cerr << "Restriccion violada: DAG \"" << dags[i].name << "\" no es unico\n";
            return false;
        }
    }

    // R2: Capacidad — ningún clúster supera C_max
    // Verificamos directamente current_capacity que se mantiene actualizado
    for (const auto& cluster : sol.clusters) {
        // Creamos un clúster temporal vacío y simulamos agregar todos sus DAGs
        // para reutilizar check_capacity_constraint de constraints.cpp
        Cluster temp;
        temp.current_capacity = 0;
        for (int dag_idx : cluster.dags_indices) {
            if (!check_capacity_constraint(temp, dags[dag_idx], max_capacity)) {
                cerr << "Restriccion violada: cluster " << cluster.id
                     << " supera capacidad maxima (" << max_capacity << ")\n";
                return false;
            }
            temp.current_capacity += dags[dag_idx].num_source_tables
                                   + dags[dag_idx].num_target_tables;
        }
    }

    return true;
}