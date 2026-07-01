#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H

#include "types.h"
#include <vector>

using namespace std;

// Ejecuta la metaheurística de Búsqueda Tabú para mejorar la asignación de DAGs
// max_iterations: Número máximo de iteraciones sin criterio de parada
// tabu_tenure: Cantidad de iteraciones que un movimiento permanece bloqueado (lista tabú)
Solution run_tabu_search(const Solution& initial_sol, const vector<DAG>& dags, int max_capacity, int max_iterations, int tabu_tenure);

#endif