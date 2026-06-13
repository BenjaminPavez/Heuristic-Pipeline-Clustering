#include <iostream>
#include <fstream>
#include <sstream>
#include "types.h"

std::vector<DAG> load_dags(const std::string& filename) {
    std::vector<DAG> dags;
    std::ifstream file(filename);
    std::string line, token;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        DAG current_dag;
        
        // 1. Nombre del DAG
        std::getline(ss, current_dag.name, ',');
        
        // 2. Número de tablas origen
        std::getline(ss, token, ',');
        current_dag.num_source_tables = std::stoi(token);
        
        // 3. Tablas de origen
        for (int i = 0; i < current_dag.num_source_tables; ++i) {
            std::getline(ss, token, ',');
            current_dag.source_tables.push_back(token);
        }
        
        // 4. Número de tablas destino
        std::getline(ss, token, ',');
        current_dag.num_target_tables = std::stoi(token);
        
        // 5. Tablas destino
        for (int i = 0; i < current_dag.num_target_tables; ++i) {
            std::getline(ss, token, ',');
            current_dag.target_tables.push_back(token);
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