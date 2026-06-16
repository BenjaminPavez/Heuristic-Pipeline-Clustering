#include <iostream>
#include <fstream>
#include <sstream>
#include "types.h"

std::vector<DAG> load_dags(const std::string& filename) {
    std::vector<DAG> dags;
    std::ifstream file(filename);
    std::string line, token, list_token;

    // Saltar la primera línea (cabecera del CSV)
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue; // Ignorar líneas en blanco
        
        std::stringstream ss(line);
        DAG current_dag;
        
        // 1. Nombre del DAG (Separado por ';')
        std::getline(ss, current_dag.name, ';');
        
        // 2. Número de tablas que Alimenta (Targets/Outputs)
        std::getline(ss, token, ';');
        current_dag.num_target_tables = std::stoi(token);
        
        // 3. Nombres de tablas que Alimenta (Separadas por ',')
        std::getline(ss, token, ';');
        std::stringstream ss_targets(token);
        while (std::getline(ss_targets, list_token, ',')) {
            current_dag.target_tables.push_back(list_token);
        }
        
        // 4. Número de tablas de las que se Alimenta (Sources/Inputs)
        std::getline(ss, token, ';');
        current_dag.num_source_tables = std::stoi(token);
        
        // 5. Nombres de tablas de las que se Alimenta (Separadas por ',')
        std::getline(ss, token, ';');
        std::stringstream ss_sources(token);
        while (std::getline(ss_sources, list_token, ',')) {
            current_dag.source_tables.push_back(list_token);
        }
        
        dags.push_back(current_dag);
    }
    return dags;
}

void save_solution_to_csv(const Solution& sol, const std::vector<DAG>& dags, const std::string& filename) {
    std::ofstream file(filename);
    file << "cluster_id,dag_name\n"; // Cabecera
    
    for (const auto& cluster : sol.clusters) {
        for (int dag_idx : cluster.dags_indices) {
            file << cluster.id << "," << dags[dag_idx].name << "\n";
        }
    }
    file.close();
}