#ifndef REPORTING_H
#define REPORTING_H

#include "types.h"
#include <vector>
#include <string>

using namespace std;

// Genera el ranking de tablas críticas del ecosistema completo
vector<CriticalTable> build_critical_table_ranking(const vector<DAG>& dags);

// Genera el reporte de cohesión y semáforo por clúster
vector<ClusterReport> build_cluster_reports(const Solution& sol, const vector<DAG>& dags);

// Imprime en consola un resumen ejecutivo
void print_summary(const Solution& sol,
                   const vector<DAG>& dags,
                   const vector<ClusterReport>& reports,
                   const vector<CriticalTable>& critical_tables,
                   int top_n_tables = 15);

// Exporta el ranking de tablas críticas a CSV
void save_critical_tables_csv(const vector<CriticalTable>& tables, const string& filename);

// Exporta el reporte de clústeres a CSV
void save_cluster_report_csv(const vector<ClusterReport>& reports, const string& filename);

#endif