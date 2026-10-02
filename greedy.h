#ifndef GREEDY_H
#define GREEDY_H

#include "types.h"
#include <vector>

using namespace std;


// Ejecuta el algoritmo Greedy para generar una solucion inicial de asignacion de DAGs a clusters
Solution run_greedy(const vector<DAG>& dags, const vector<TableImportance>& optional_weights, int max_capacity);


#endif