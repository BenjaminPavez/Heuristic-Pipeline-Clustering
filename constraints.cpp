#include "types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <algorithm>

using namespace std;


// Funciones externas
extern int calculate_affinity(const DAG& dag1, const DAG& dag2);



/*
La funcion representa la restriccion de capacidad maxima.

Parametros :
   const Cluster& cluster : Estructura Cluster que contiene un cluster con DAGs.
   const DAG& new_dag : Estructura DAG que contiene el DAG a agregar al cluster.
   int max_tables_per_cluster : Entero con la capacidad maxima de los cluster.

Retorno :
   bool : Indica si al agregar un nuevo DAG al cluster cumple con el limite C_max o no.

*/
bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster){
    int new_dag_weight = new_dag.num_source_tables + new_dag.num_target_tables;
    return (cluster.current_capacity + new_dag_weight) <= max_tables_per_cluster;
}



/*
La funcion representa la restriccion de unicidad.

Parametros :
   const Solution& sol : Estructura Solution que contiene una solucion.
   int dag_index : Entero con el identificador del dag a revisar.

Retorno :
   bool : Indica si el DAG esta unicamente en un cluster de la solucion o no.

*/
bool check_uniqueness_constraint(const Solution& sol, int dag_index){
    int occurrences = 0;
    for (const auto& cluster : sol.clusters)
        for (int id : cluster.dags_indices)
            if (id == dag_index) occurrences++;
    return occurrences <= 1;
}



/*
La funcion representa la restriccion de integridad del DAG.

Parametros :
   const DAG& dag : Estructura DAG que contiene un DAG.

Retorno :
   bool : Indica si el DAG tiene al menos una tabla source y una tabla target o no.

*/
bool check_dag_integrity(const DAG& dag){
    return !dag.source_tables.empty() && !dag.target_tables.empty();
}



/*
La funcion retorna si un DAG tiene afinidad positiva con algun DAG que ya este en el cluster, se utiliza en reporting.

Parametros :
   const Cluster& cluster : Estructura Cluster que contiene un cluster de la solucion.
   const DAG& candidate : Estructura DAG que contiene al DAG candidato.
   const vector<DAG>& all_dags : Vector de estructura DAG que contiene todos los DAGs del archivo de entrada.

Retorno :
   bool : Indica si el DAG tiene afinidad positiva con algun DAG que ya este en el cluster o no.

*/
bool check_connectivity_constraint(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    if (cluster.dags_indices.empty()) return true;
    for (int idx : cluster.dags_indices)
        if (calculate_affinity(candidate, all_dags[idx]) > 0)
            return true;
    return false;
}



/*
La funcion retorna la cantidad de tablas target de un DAG candidato que ya son escritas por otro DAG dentro del cluster.

Parametros :
   const Cluster& cluster : Estructura Cluster que contiene un cluster de la solucion.
   const DAG& candidate : Estructura DAG que contiene al DAG candidato.
   const vector<DAG>& all_dags : Vector de estructura DAG que contiene todos los DAGs del archivo de entrada.

Retorno :
   int : Cantidad de tablas que presentan el conflicto de escritura en el DAG.

*/
int count_target_conflicts(const Cluster& cluster, const DAG& candidate, const vector<DAG>& all_dags){
    unordered_map<string,int> existing_targets;
    for (int idx : cluster.dags_indices)
        for (const auto& t : all_dags[idx].target_tables)
            existing_targets[t]++;
    int conflicts = 0;
    for (const auto& t : candidate.target_tables)
        if (existing_targets.count(t)) conflicts++;
    return conflicts;
}



/*
La funcion revisa si el movimiento que se va a realizar es factible.

Parametros :
   const Cluster& target_cluster : Estructura Cluster que contiene un cluster de destino.
   const DAG& dag_to_move : Estructura DAG que contiene el DAG que se quiere mover.
   const vector<DAG>& all_dags : Vector de estructura DAG que contiene todos los DAGs del archivo de entrada.
   int max_capacity : Entero con la capacidad maxima de los cluster.

Retorno :
   bool : Indica si el movimiento a realizar es factible.

*/
bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const vector<DAG>& all_dags [[maybe_unused]], int max_capacity){
    return check_capacity_constraint(target_cluster, dag_to_move, max_capacity);
}