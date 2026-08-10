#include <iostream>
#include <fstream>
#include <sstream>
#include "types.h"

using namespace std;

// Funciones externas
extern bool check_dag_integrity(const DAG& dag);


/*
La funcion lee y vuelca en las estructuras definidas en types.h los DAGs y sus tablas desde un archivo CSV de entrada.

Parametros :
   const string& filename : Ubicacion del archivo .csv con los DAGs y sus tablas.

Retorno :
   vector<DAG> dags : Vector de tipo DAG con los DAGs y sus tablas cargadas desde el archivo CSV.

*/
vector<DAG> load_dags(const string& filename) {
    vector<DAG> dags;
    ifstream file(filename);
    string line, token, list_token;
    int skipped = 0;

    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) continue;
        
        stringstream ss(line);
        DAG current_dag;
        
        // Nombre del DAG
        getline(ss, current_dag.name, ';');
        
        // Num de tablas de destino
        getline(ss, token, ';');
        current_dag.num_target_tables = stoi(token);
        
        // Nombres de tablas que Alimenta
        getline(ss, token, ';');
        stringstream ss_targets(token);
        while (getline(ss_targets, list_token, ',')) {
            current_dag.target_tables.push_back(list_token);
        }
        
        // Num de tablas de origen
        getline(ss, token, ';');
        current_dag.num_source_tables = stoi(token);
        
        // Nombres de tablas de las que se Alimenta
        getline(ss, token, ';');
        stringstream ss_sources(token);
        while (getline(ss_sources, list_token, ',')) {
            current_dag.source_tables.push_back(list_token);
        }

        if (!check_dag_integrity(current_dag)) {
            cerr << "[WARNING] DAG \"" << current_dag.name << "\" ignorado: debe tener al menos una tabla source y una target.\n";
            skipped++;
            continue;
        }

        dags.push_back(current_dag);
    }

    if (skipped > 0)
        cerr << "[WARNING] Total DAGs ignorados por integridad: " << skipped << "\n";

    return dags;
}



/*
La funcion lee y vuelca en las estructuras definidas en types.h los tablas con sus pesos opcionales.

Parametros :
   const string& filename : Ubicacion del archivo .csv con las tablas y sus pesos opcionales.

Retorno :
   vector<TableImportance> optional_weights : Vector de tipo TableImportance con las tablas y sus pesos opcionales cargados desde el archivo CSV.

*/
vector<TableImportance> load_optional_weights(const string& filename) {
    vector<TableImportance> optional_weights;
    ifstream file(filename);
    string line, token, list_token;

    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) continue;
        
        stringstream ss(line);
        TableImportance current_table_importance;
        
        // Nombre de la tabla
        getline(ss, current_table_importance.name, ';');
        
        // Peso extra (este peso es definido como >= 0)
        getline(ss, token, ';');
        current_table_importance.extraWeight = stoi(token);

        optional_weights.push_back(current_table_importance);
    }

    return optional_weights;
}



/*
La funcion lee y vuelca en las estructuras definidas en types.h los tablas con sus pesos opcionales.

Parametros :
   const string& filename : Ubicacion del archivo .csv con las tablas y sus pesos opcionales.

Retorno :
   vector<TableImportance> optional_weights : Vector de tipo TableImportance con las tablas y sus pesos opcionales cargados desde el archivo CSV.

*/
void save_solution_to_csv(const Solution& sol, const vector<DAG>& dags, const string& filename) {
    ofstream file(filename);
    file << "cluster_id,dag_name\n";
    
    for (const auto& cluster : sol.clusters) {
        for (int dag_idx : cluster.dags_indices) {
            file << cluster.id << "," << dags[dag_idx].name << "\n";
        }
    }
    file.close();
}