#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

// Pesos para la función de fitness
// Ajustar estos valores para cambiar el comportamiento del algoritmo
static const int WEIGHT_SHARED_TABLE      =  1; // Tabla genérica compartida entre dos DAGs
static const int WEIGHT_LINEAGE_RELATION  =  3; // Relación productor→consumidor (linaje directo)
static const int WEIGHT_WRITE_CONFLICT    = -4; // Penalización: dos DAGs escriben la misma tabla target

// Representa un flujo de orquestación
struct DAG {
    std::string name;
    int num_source_tables;
    std::vector<std::string> source_tables;
    int num_target_tables;
    std::vector<std::string> target_tables;
};

// Representa el Dominio de Datos (Caja lógica)
struct Cluster {
    int id;
    int current_capacity;
    std::vector<int> dags_indices;  // Índices de los DAGs asignados a esta caja
    int write_conflicts = 0;        // Cantidad de conflictos de escritura detectados en este clúster
};

// Score de semáforo por clúster (generado por el módulo de reporting)
struct ClusterReport {
    int cluster_id;
    int num_dags;
    int capacity_used;
    int internal_affinity;    // Afinidad total interna del clúster
    int write_conflicts;      // Conflictos de escritura detectados
    int lineage_pairs;        // Pares con relación productor→consumidor
    double cohesion_score;    // Score normalizado 0-100
    std::string semaphore;    // "GREEN", "YELLOW", "RED"
    std::vector<std::string> unique_tables; // Todas las tablas únicas cubiertas
};

// Tabla crítica con su frecuencia de aparición
struct CriticalTable {
    std::string name;
    int frequency;            // En cuántos DAGs aparece
    int as_source;            // Cuántas veces como tabla de lectura
    int as_target;            // Cuántas veces como tabla de escritura
    bool is_contested;        // true si más de un DAG escribe en ella
};

// Estructura de la solución completa
struct Solution {
    std::vector<Cluster> clusters;
    int fitness_score; // Calculado por la afinidad de tablas con penalizaciones
};

#endif