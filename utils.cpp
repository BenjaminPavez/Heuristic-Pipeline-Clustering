#include <iostream>
#include <fstream>
#include <sstream>
#include "types.h"

using namespace std;

extern bool check_dag_integrity(const DAG& dag);

vector<DAG> load_dags(const string& filename) {
    vector<DAG> dags;
    ifstream file(filename);
    string line, token, list_token;
    int skipped = 0;

    // Saltar la primera línea (cabecera del CSV)
    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) continue; // Ignorar líneas en blanco
        
        stringstream ss(line);
        DAG current_dag;
        
        // 1. Nombre del DAG (Separado por ';')
        getline(ss, current_dag.name, ';');
        
        // 2. Número de tablas que Alimenta (Targets/Outputs)
        getline(ss, token, ';');
        current_dag.num_target_tables = stoi(token);
        
        // 3. Nombres de tablas que Alimenta (Separadas por ',')
        getline(ss, token, ';');
        stringstream ss_targets(token);
        while (getline(ss_targets, list_token, ',')) {
            current_dag.target_tables.push_back(list_token);
        }
        
        // 4. Número de tablas de las que se Alimenta (Sources/Inputs)
        getline(ss, token, ';');
        current_dag.num_source_tables = stoi(token);
        
        // 5. Nombres de tablas de las que se Alimenta (Separadas por ',')
        getline(ss, token, ';');
        stringstream ss_sources(token);
        while (getline(ss_sources, list_token, ',')) {
            current_dag.source_tables.push_back(list_token);
        }

        // Validación de integridad: descartar DAGs sin tablas source o target
        if (!check_dag_integrity(current_dag)) {
            cerr << "WARNING: DAG \"" << current_dag.name
                 << "\" ignorado: debe tener al menos una tabla source y una target.\n";
            skipped++;
            continue;
        }

        dags.push_back(current_dag);
    }

    if (skipped > 0)
        cerr << "Total DAGs ignorados por integridad: " << skipped << "\n";

    return dags;
}

void save_solution_to_csv(const Solution& sol, const vector<DAG>& dags, const string& filename) {
    ofstream file(filename);
    file << "cluster_id,dag_name\n"; // Cabecera
    
    for (const auto& cluster : sol.clusters) {
        for (int dag_idx : cluster.dags_indices) {
            file << cluster.id << "," << dags[dag_idx].name << "\n";
        }
    }
    file.close();
}