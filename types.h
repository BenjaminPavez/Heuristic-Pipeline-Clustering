#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

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
    std::vector<int> dags_indices; // Índices de los DAGs asignados a esta caja
};

// Estructura de la solución completa
struct Solution {
    std::vector<Cluster> clusters;
    int fitness_score; // Calculado por la afinidad de tablas
};

#endif