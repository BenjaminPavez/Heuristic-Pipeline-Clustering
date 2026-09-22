#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

using namespace std;


// Funciones externas
extern bool check_uniqueness_constraint(const Solution& sol, int dag_index);
extern bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster);



/*
La funcion retorna el peso extra de una tabla si esta en optional_weight o 0 si no.

Parametros :
   const string& table_name : String con el nombre de la tabla.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con el peso de las tablas.

Retorno :
   int : Peso extra de la tabla.

*/
static int get_table_extra_weight(const string& table_name, const vector<TableImportance>& optional_weights) {
    for (const auto& ti : optional_weights)
        if (ti.name == table_name) return ti.extraWeight;
    return 0;
}



/*
La funcion calcula la afinidad entre dos DAGs de acuerdo a las reglas detalladas en la memoria.

Parametros :
   const DAG& dag1 : Estructura DAG que contiene la informacion particular de un DAG.
   const DAG& dag2 : Estructura DAG que contiene la informacion particular de un DAG.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con el peso de las tablas.

Retorno :
   int : Afinidad entre los DAGs dag1 y dag2.

*/
int calculate_affinity(const DAG& dag1, const DAG& dag2, const vector<TableImportance>& optional_weights) {
    int score = 0;

    unordered_set<string> targets1(dag1.target_tables.begin(), dag1.target_tables.end());
    unordered_set<string> targets2(dag2.target_tables.begin(), dag2.target_tables.end());
    unordered_set<string> sources1(dag1.source_tables.begin(), dag1.source_tables.end());
    unordered_set<string> sources2(dag2.source_tables.begin(), dag2.source_tables.end());

    unordered_set<string> counted;

    // 1. Conflictos de escritura: penalizacion mayor si la tabla es critica
    for (const auto& t : targets1) {
        if (targets2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_WRITE_CONFLICT - w;
            counted.insert(t);
        }
    }

    // 2. Linaje directo: dag1 escribe lo que dag2 lee (mayor afinidad si tabla es critica)
    for (const auto& t : targets1) {
        if (!counted.count(t) && sources2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_LINEAGE_RELATION + w;
            counted.insert(t);
        }
    }

    // 3. Linaje inverso: dag2 escribe lo que dag1 lee (mayor afinidad si tabla es critica)
    for (const auto& t : targets2) {
        if (!counted.count(t) && sources1.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_LINEAGE_RELATION + w;
            counted.insert(t);
        }
    }

    // 4. Tablas fuente compartidas (mayor afinidad si tabla es critica)
    for (const auto& t : sources1) {
        if (!counted.count(t) && sources2.count(t)) {
            int w = get_table_extra_weight(t, optional_weights);
            score += WEIGHT_SHARED_TABLE + w;
            counted.insert(t);
        }
    }

    return score;
}



/*
La funcion calcula la afinidad entre dos DAGs de acuerdo a las reglas detalladas en la memoria (uso para reporting cuando no hay tablas importantes).

Parametros :
   const DAG& dag1 : Estructura DAG que contiene la informacion particular de un DAG.
   const DAG& dag2 : Estructura DAG que contiene la informacion particular de un DAG.

Retorno :
   int : Afinidad entre los DAGs dag1 y dag2.

*/
int calculate_affinity(const DAG& dag1, const DAG& dag2) {
    static const vector<TableImportance> empty;
    return calculate_affinity(dag1, dag2, empty);
}



/*
La funcion calcula la afinidad total para obtener el valor de la solucion.

Parametros :
   Solution& sol : Estructura Solution que contiene una solucion.
   const vector<DAG>& all_dags : Vector con los DAGs extraidos del archivo de entrada.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con el peso de las tablas.

Retorno :
   int : Valor de la solucion.

*/
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



/*
La funcion calcula la afinidad total para obtener el valor de la solucion (const para Tabu Search).

Parametros :
   const Solution& sol : Estructura Solution que contiene una solucion.
   const vector<DAG>& all_dags : Vector con los DAGs extraidos del archivo de entrada.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con el peso de las tablas.

Retorno :
   int : Valor de la solucion.

*/
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



/*
La funcion calcula la afinidad total para obtener el valor de la solucion (sin tablas importantes).

Parametros :
   Solution& sol : Estructura Solution que contiene una solucion.
   const vector<DAG>& all_dags : Vector con los DAGs extraidos del archivo de entrada.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con el peso de las tablas.

Retorno :
   int : Valor de la solucion.

*/
int evaluate_solution(Solution& sol, const vector<DAG>& all_dags) {
    static const vector<TableImportance> empty;
    return evaluate_solution(sol, all_dags, empty);
}



/*
La funcion calcula la afinidad total para obtener el valor de la solucion (const para Tabu Search y sin tablas importantes).

Parametros :
   const Solution& sol : Estructura Solution que contiene una solucion.
   const vector<DAG>& all_dags : Vector con los DAGs extraidos del archivo de entrada.

Retorno :
   int : Valor de la solucion.

*/
int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags) {
    static const vector<TableImportance> empty;
    return evaluate_solution(sol, all_dags, empty);
}



/*
La funcion valida que una solución completa cumpla todas las restricciones duras.

Parametros :
   const Solution& sol : Estructura Solution que contiene una solucion.
   const vector<DAG>& dags : Vector con los DAGs.
   int max_capacity : Entero con la capacidad maxima de los cluster.

Retorno :
   bool : Indica si la solucion es factible o no.

*/
bool is_feasible_solution(const Solution& sol, const vector<DAG>& dags, int max_capacity) {
    // R1: Unicidad - cada DAG debe aparecer una vez
    for (size_t i = 0; i < dags.size(); ++i) {
        if (!check_uniqueness_constraint(sol, (int)i)) {
            cerr << "Restriccion violada: DAG \"" << dags[i].name << "\" no es unico\n";
            return false;
        }
    }

    // R2: Capacidad - ningún cluster supera C_max
    for (const auto& cluster : sol.clusters) {
        // Creamos un cluster temporal vacio y simulamos agregar todos sus DAGs
        Cluster temp;
        temp.current_capacity = 0;
        for (int dag_idx : cluster.dags_indices) {
            if (!check_capacity_constraint(temp, dags[dag_idx], max_capacity)) {
                cerr << "Restriccion violada: cluster " << cluster.id << " supera capacidad maxima (" << max_capacity << ")\n";
                return false;
            }
            temp.current_capacity += dags[dag_idx].num_source_tables + dags[dag_idx].num_target_tables;
        }
    }

    return true;
}