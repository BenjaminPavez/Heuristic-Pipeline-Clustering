#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

using namespace std;


static const int WEIGHT_SHARED_TABLE      =  1;
static const int WEIGHT_LINEAGE_RELATION  =  3;
static const int WEIGHT_WRITE_CONFLICT    = -4;


// Representa un DAG de Apache Airflow
struct DAG {
    string name;
    int num_source_tables;
    vector<string> source_tables;
    int num_target_tables;
    vector<string> target_tables;
};


// Representa los pesos a tablas importantes
struct TableImportance {
    string name;
    int extraWeight;
};


// Buscar por nombre de tabla
struct FindByName {
    const string name;
    FindByName(const string& name) : name(name) {}
    bool operator()(const TableImportance& t) const { 
        return t.name == name; 
    }
};


// Representa el Dominio de Datos
struct Cluster {
    int id;
    int current_capacity;
    vector<int> dags_indices;
    int write_conflicts = 0;
};


// Score de semaforo por cluster (reporting)
struct ClusterReport {
    int cluster_id;
    int num_dags;
    int capacity_used;
    int internal_affinity;
    int write_conflicts;
    int lineage_pairs;
    double cohesion_score;
    string semaphore;
    vector<string> unique_tables;
};


// Tabla critica con su frecuencia de aparicion
struct CriticalTable {
    string name;
    int frequency;
    int as_source;
    int as_target;
    bool is_contested;
};


// Estructura de la solución completa
struct Solution {
    vector<Cluster> clusters;
    int fitness_score;
};


#endif