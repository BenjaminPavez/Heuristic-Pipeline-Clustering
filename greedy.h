#ifndef GREEDY_H
#define GREEDY_H

#include "types.h"
#include <vector>

// Funcion principal que orquesta el empaquetamiento inicial de los DAGs
Solution run_greedy(const std::vector<DAG>& dags, int max_capacity);

#endif