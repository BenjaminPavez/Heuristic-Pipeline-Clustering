#ifndef REPORTING_H
#define REPORTING_H

#include "types.h"
#include <vector>
#include <string>

// Genera el ranking de tablas críticas del ecosistema completo
std::vector<CriticalTable> build_critical_table_ranking(const std::vector<DAG>& dags);

// Genera el reporte de cohesión y semáforo por clúster
std::vector<ClusterReport> build_cluster_reports(const Solution& sol, const std::vector<DAG>& dags);

// Imprime en consola un resumen ejecutivo
void print_summary(const Solution& sol,
                   const std::vector<DAG>& dags,
                   const std::vector<ClusterReport>& reports,
                   const std::vector<CriticalTable>& critical_tables,
                   int top_n_tables = 15);

// Exporta el ranking de tablas críticas a CSV
void save_critical_tables_csv(const std::vector<CriticalTable>& tables, const std::string& filename);

// Exporta el reporte de clústeres a CSV
void save_cluster_report_csv(const std::vector<ClusterReport>& reports, const std::string& filename);

#endif