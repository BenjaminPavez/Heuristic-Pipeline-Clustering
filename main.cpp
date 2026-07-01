#include <iostream>
#include <chrono>
#include "types.h"
#include "utils.h"
#include "greedy.h"
#include "tabu_search.h"
#include "reporting.h"

using namespace std;

// Declaración de la sobrecarga no-const de evaluate_solution para actualizar write_conflicts
extern int evaluate_solution(Solution& sol, const vector<DAG>& all_dags);

int main(){
    auto start = chrono::high_resolution_clock::now();

    // -----------------------------------------------------------------------
    // 1. Cargar datos
    // -----------------------------------------------------------------------
    vector<DAG> dags = load_dags("Instances/dags_100.csv");
    cout << "DAGs cargados: " << dags.size() << "\n";

    // -----------------------------------------------------------------------
    // 2. Solución Constructiva (Greedy)
    //    max_capacity = 50 tablas I/O por clúster
    // -----------------------------------------------------------------------
    const int MAX_CAPACITY  = 50;
    const int MAX_ITER      = 1000;
    const int TABU_TENURE   = 10;

    Solution initial_sol = run_greedy(dags, MAX_CAPACITY);
    cout << "Greedy: " << initial_sol.clusters.size() << " dominios generados\n";

    // -----------------------------------------------------------------------
    // 3. Mejoramiento Heurístico (Búsqueda Tabú)
    // -----------------------------------------------------------------------
    Solution best_sol = run_tabu_search(initial_sol, dags, MAX_CAPACITY, MAX_ITER, TABU_TENURE);

    // Recalcular con sobrecarga no-const para actualizar write_conflicts por clúster
    best_sol.fitness_score = evaluate_solution(best_sol, dags);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    // -----------------------------------------------------------------------
    // 4. Reporting
    // -----------------------------------------------------------------------
    auto critical_tables = build_critical_table_ranking(dags);
    auto cluster_reports = build_cluster_reports(best_sol, dags);

    print_summary(best_sol, dags, cluster_reports, critical_tables, 15);

    cout << "Tiempo de ejecucion: " << duration.count() << " segundos\n\n";

    // -----------------------------------------------------------------------
    // 5. Exportar resultados
    // -----------------------------------------------------------------------
    save_solution_to_csv(best_sol, dags,         "Solved/dags_100_solved.csv");
    save_critical_tables_csv(critical_tables,     "Solved/critical_tables.csv");
    save_cluster_report_csv(cluster_reports,      "Solved/cluster_report.csv");

    cout << "Archivos exportados en Solved/\n";
    cout << "  - dags_100_solved.csv    -> asignacion DAG -> dominio\n";
    cout << "  - critical_tables.csv    -> ranking de tablas criticas\n";
    cout << "  - cluster_report.csv     -> score y semaforo por dominio\n";

    return 0;
}