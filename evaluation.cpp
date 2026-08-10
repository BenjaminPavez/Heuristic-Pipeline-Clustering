#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------------
// get_table_extra_weight
// Retorna el peso extra de una tabla si está en optional_weights, o 0 si no.
// ---------------------------------------------------------------------------
static int get_table_extra_weight(const string& table_name, const vector<TableImportance>& optional_weights) {
    for (const auto& ti : optional_weights)
        if (ti.name == table_name) return ti.extraWeight;
    return 0;
}

// ---------------------------------------------------------------------------
// calculate_affinity
// Calcula la afinidad entre dos DAGs distinguiendo tres casos.
// Si una tabla tiene peso extra en optional_weights, ese peso se suma
// al aporte base de λ, aumentando la afinidad entre DAGs que la comparten.
//
//  (λ_L + w(t)) por cada relación productor→consumidor (linaje directo)
//  (λ_S + w(t)) por cada tabla fuente compartida genérica
//  (λ_C - w(t)) por cada tabla que AMBOS escriben (penalización mayor si es crítica)
// ---------------------------------------------------------------------------
int calculate_affinity(const DAG& dag1, const DAG& dag2, const vector<TableImportance>& optional_weights) {
    int score = 0;

    unordered_set<string> targets1(dag1.target_tables.begin(), dag1.target_tables.end());
    unordered_set<string> targets2(dag2.target_tables.begin(), dag2.target_tables.end());
    unordered_set<string> sources1(dag1.source_tables.begin(), dag1.source_tables.end());
    unordered_set<string> sources2(dag2.source_tables.begin(), dag2.source_tables.end());

    unordered_set<string> counted;

    // 1. Conflictos de escritura: penalización mayor si la tabla es crítica
    for (const auto& t : targets1) {
        if (targets2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_WRITE_CONFLICT - w;
            counted.insert(t);
        }
    }

    // 2. Linaje directo: dag1 escribe lo que dag2 lee (mayor afinidad si tabla es crítica)
    for (const auto& t : targets1) {
        if (!counted.count(t) && sources2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_LINEAGE_RELATION + w;
            counted.insert(t);
        }
    }

    // 3. Linaje inverso: dag2 escribe lo que dag1 lee (mayor afinidad si tabla es crítica)
    for (const auto& t : targets2) {
        if (!counted.count(t) && sources1.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_LINEAGE_RELATION + w;
            counted.insert(t);
        }
    }

    // 4. Tablas fuente compartidas (mayor afinidad si tabla es crítica)
    for (const auto& t : sources1) {
        if (!counted.count(t) && sources2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_SHARED_TABLE + w;
            counted.insert(t);
        }
    }

    return score;
}

// Sobrecarga sin optional_weights para compatibilidad con reporting
int calculate_affinity(const DAG& dag1, const DAG& dag2) {
    static const vector<TableImportance> empty;
    return calculate_affinity(dag1, dag2, empty);
}

// ---------------------------------------------------------------------------
// evaluate_solution: Suma la afinidad cruzada de todos los pares dentro
// de cada clúster. También actualiza write_conflicts por clúster.
// ---------------------------------------------------------------------------
int evaluate_solution(Solution& sol, const vector<DAG>& all_dags, const vector<TableImportance>& optional_weights) {
    int total_fitness = 0;

    for (auto& cluster : sol.clusters) {
        cluster.write_conflicts = 0;

        unordered_map<string, int> target_writers;
        for (int idx : cluster.dags_indices)
            for (const auto& t : all_dags[idx].target_tables)
                target_writers[t]++;
        for (const auto& [table, count] : target_writers)
            if (count > 1) cluster.write_conflicts += (count - 1);

        for (size_t i = 0; i < cluster.dags_indices.size(); ++i)
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j)
                total_fitness += calculate_affinity(
                    all_dags[cluster.dags_indices[i]],
                    all_dags[cluster.dags_indices[j]],
                    optional_weights);
    }
    return total_fitness;
}

// Sobrecarga const para Tabu Search (sin actualizar write_conflicts)
int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags, const vector<TableImportance>& optional_weights) {
    int total_fitness = 0;
    for (const auto& cluster : sol.clusters)
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i)
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j)
                total_fitness += calculate_affinity(
                    all_dags[cluster.dags_indices[i]],
                    all_dags[cluster.dags_indices[j]],
                    optional_weights);
    return total_fitness;
}

// Sobrecargas sin optional_weights para compatibilidad con reporting
int evaluate_solution(Solution& sol, const vector<DAG>& all_dags) {
    static const vector<TableImportance> empty;
    return evaluate_solution(sol, all_dags, empty);
}

int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags) {
    static const vector<TableImportance> empty;
    return evaluate_solution(sol, all_dags, empty);
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