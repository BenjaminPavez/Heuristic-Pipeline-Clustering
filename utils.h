#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <string>
#include <vector>

using namespace std;

// Carga los DAGs y sus dependencias desde un archivo CSV de entrada
vector<DAG> load_dags(const string& filename);

// Carga los pesos opcionales de tablas importantes desde un archivo CSV de entrada
vector<TableImportance> load_optional_weights(const string& filename);

// Exporta la solución final (los dominios lógicos) a un archivo CSV para ser leído por Airflow
void save_solution_to_csv(const Solution& sol, const vector<DAG>& dags, const string& filename);

#endif