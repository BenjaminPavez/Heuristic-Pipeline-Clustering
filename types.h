#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

using namespace std;


static const int WEIGHT_SHARED_TABLE      =  1;
static const int WEIGHT_LINEAGE_RELATION  =  3;
static const int WEIGHT_WRITE_CONFLICT    = -4;


/*
La funcion lee y vuelca en las estructuras definidas en types.h los DAGs y sus tablas desde un archivo CSV de entrada.

Parametros :
   const string& filename : Ubicacion del archivo .csv con los DAGs y sus tablas.

Retorno :
   vector<DAG> dags : Vector de tipo DAG con los DAGs y sus tablas cargadas desde el archivo CSV.

*/
struct DAG {
    string name;
    int num_source_tables;
    vector<string> source_tables;
    int num_target_tables;
    vector<string> target_tables;
};

// Parametro opcional a aquellas tablas que por experiencia o uso se consideran mas importantes, por lo que el peso modifica la afinidad de los DAGs que las contienen
struct TableImportance {
    string name;
    int extraWeight;
};

struct FindByName {
    const string name;
    FindByName(const string& name) : name(name) {}
    bool operator()(const TableImportance& t) const { 
        return t.name == name; 
    }
};

// Representa el Dominio de Datos (Caja lógica)
struct Cluster {
    int id;
    int current_capacity;
    vector<int> dags_indices;  // Índices de los DAGs asignados a esta caja
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
    string semaphore;    // "GREEN", "YELLOW", "RED"
    vector<string> unique_tables; // Todas las tablas únicas cubiertas
};

// Tabla critica con su frecuencia de aparición
struct CriticalTable {
    string name;
    int frequency;            // En cuántos DAGs aparece
    int as_source;            // Cuántas veces como tabla de lectura
    int as_target;            // Cuántas veces como tabla de escritura
    bool is_contested;        // true si más de un DAG escribe en ella
};

// Estructura de la solución completa
struct Solution {
    vector<Cluster> clusters;
    int fitness_score; // Calculado por la afinidad de tablas con penalizaciones
};

#endif