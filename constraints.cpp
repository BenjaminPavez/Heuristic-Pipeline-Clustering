#include "types.h"
#include <vector>
#include <iostream>

// 1. Restricción de Capacidad del Clúster (Límite de I/O)
// Verifica si agregar un nuevo DAG a un dominio supera el límite de tablas permitidas
bool check_capacity_constraint(const Cluster& cluster, const DAG& new_dag, int max_tables_per_cluster) {
    // El "peso" del DAG se puede definir por la cantidad de tablas que lee y escribe
    int new_dag_weight = new_dag.num_source_tables + new_dag.num_target_tables;

    // Si la capacidad actual del clúster más el nuevo DAG supera el límite del servidor
    if (cluster.current_capacity + new_dag_weight > max_tables_per_cluster) {
        return false; // Restricción violada: El clúster está saturado
    }
    return true; // Es factible agregarlo
}

// 2. Restricción de Unicidad (Prevención de ejecuciones simultáneas y redundancia)
// Un DAG no puede ser asignado a más de un Dominio de Datos (clúster) al mismo tiempo
bool check_uniqueness_constraint(const Solution& sol, int dag_index) {
    int occurrences = 0;
    
    for (const auto& cluster : sol.clusters) {
        for (int id : cluster.dags_indices) {
            if (id == dag_index) {
                occurrences++;
            }
        }
    }
    
    // Si el DAG aparece más de una vez, la solución es infactible (genera redundancia)
    return occurrences <= 1; 
}

// Función orquestadora que valida un movimiento antes de realizarlo (muy útil para Tabú Search)
bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const Solution& current_sol, int max_capacity) {
    
    // Si viola la capacidad del servidor, descartamos el movimiento inmediatamente
    if (!check_capacity_constraint(target_cluster, dag_to_move, max_capacity)) {
        return false; 
    }
    
    // (Opcional) Aquí podrías agregar más restricciones duras en el futuro, 
    // como evitar que dos DAGs conflictivos se ejecuten en la misma caja.
    
    return true;
}