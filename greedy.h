#ifndef GREEDY_H
#define GREEDY_H

#include "types.h"
#include <vector>

using namespace std;

Solution run_greedy(const vector<DAG>& dags, const vector<TableImportance>& optional_weights, int max_capacity);

#endif