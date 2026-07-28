#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <string>

using namespace std;

// ---------------------------------------------------------------------------
// calculate_affinity
// Distingue tres tipos de relación entre dos DAGs:
//   +WEIGHT_LINEAGE_RELATION  : dag1 escribe lo que dag2 lee (o viceversa)
//   +WEIGHT_SHARED_TABLE      : ambos leen la misma tabla fuente
//   +WEIGHT_WRITE_CONFLICT    : ambos escriben en la misma tabla (negativo)
// ---------------------------------------------------------------------------
int calculate_affinity(const DAG& dag1, const DAG& dag2) {
    int score = 0;

    unordered_set<string> targets1(dag1.target_tables.begin(), dag1.target_tables.end());
    unordered_set<string> targets2(dag2.target_tables.begin(), dag2.target_tables.end());
    unordered_set<string> sources1(dag1.source_tables.begin(), dag1.source_tables.end());
    unordered_set<string> sources2(dag2.source_tables.begin(), dag2.source_tables.end());

    unordered_set<string> counted;

    // 1. Conflictos de escritura (penalización)
    for (const auto& t : targets1)
        if (targets2.count(t)) { score += WEIGHT_WRITE_CONFLICT; counted.insert(t); }

    // 2. Linaje directo: dag1 escribe lo que dag2 lee
    for (const auto& t : targets1)
        if (!counted.count(t) && sources2.count(t)) { score += WEIGHT_LINEAGE_RELATION; counted.insert(t); }

    // 3. Linaje inverso: dag2 escribe lo que dag1 lee
    for (const auto& t : targets2)
        if (!counted.count(t) && sources1.count(t)) { score += WEIGHT_LINEAGE_RELATION; counted.insert(t); }

    // 4. Tablas fuente compartidas
    for (const auto& t : sources1)
        if (!counted.count(t) && sources2.count(t)) { score += WEIGHT_SHARED_TABLE; counted.insert(t); }

    return score;
}

// ---------------------------------------------------------------------------
// is_feasible_solution
// Valida que una solución completa cumpla todas las restricciones duras.
// Se llama en main.cpp después del greedy y después del tabu search
// para garantizar que los algoritmos no produjeron soluciones inválidas.
// ---------------------------------------------------------------------------
bool is_feasible_solution(const Solution& sol, const vector<DAG>& dags, int max_capacity) {
    // R1: Unicidad — cada DAG debe aparecer exactamente una vez
    vector<int> dag_count(dags.size(), 0);
    for (const auto& cluster : sol.clusters)
        for (int idx : cluster.dags_indices) {
            if (idx < 0 || idx >= (int)dags.size()) return false;
            dag_count[idx]++;
        }
    for (size_t i = 0; i < dags.size(); ++i)
        if (dag_count[i] != 1) return false;

    // R2: Capacidad — ningún clúster supera C_max
    for (const auto& cluster : sol.clusters)
        if (cluster.current_capacity > max_capacity) return false;

    return true;
}

// ---------------------------------------------------------------------------
// evaluate_solution (no-const)
// Calcula el fitness total y actualiza write_conflicts por clúster.
// Se usa en main.cpp sobre la solución final para tener los datos
// completos antes del reporting.
// ---------------------------------------------------------------------------
int evaluate_solution(Solution& sol, const vector<DAG>& all_dags) {
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
                    all_dags[cluster.dags_indices[j]]);
    }
    return total_fitness;
}

// ---------------------------------------------------------------------------
// evaluate_solution (const)
// Versión ligera para el Tabu Search: solo calcula fitness,
// sin actualizar write_conflicts. Se llama miles de veces
// por iteración, por lo que evitar escrituras es importante.
// ---------------------------------------------------------------------------
int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags) {
    int total_fitness = 0;
    for (const auto& cluster : sol.clusters)
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i)
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j)
                total_fitness += calculate_affinity(
                    all_dags[cluster.dags_indices[i]],
                    all_dags[cluster.dags_indices[j]]);
    return total_fitness;
}