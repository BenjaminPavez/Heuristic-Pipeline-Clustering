#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <string>
#include <vector>

// Carga los DAGs y sus dependencias desde un archivo CSV de entrada
std::vector<DAG> load_dags(const std::string& filename);

// Exporta la solución final (los dominios lógicos) a un archivo CSV para ser leído por Airflow
void save_solution_to_csv(const Solution& sol, const std::vector<DAG>& dags, const std::string& filename);

#endif