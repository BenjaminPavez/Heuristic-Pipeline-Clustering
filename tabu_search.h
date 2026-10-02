#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H

#include "types.h"
#include <vector>

using namespace std;

// Ejecuta la metaheuristica de Busqueda Tabu para mejorar la asignación de DAGs
Solution run_tabu_search(const Solution& initial_sol, const vector<DAG>& dags, const vector<TableImportance>& optional_weights, int max_capacity, int max_iterations, int tabu_tenure, unsigned seed = 1);

#endif