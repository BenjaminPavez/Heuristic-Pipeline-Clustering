#include <iostream>
#include <chrono>
#include "types.h"
#include "utils.h"
#include "greedy.h"
#include "tabu_search.h"
#include "reporting.h"

using namespace std;


// Funciones externas
extern int evaluate_solution(Solution& sol, const vector<DAG>& all_dags, const vector<TableImportance>& optional_weights);
extern bool is_feasible_solution(const Solution& sol, const vector<DAG>& dags, int max_capacity);



int main(){
    auto start = chrono::high_resolution_clock::now();

    // Cargar datos
    vector<DAG> dags = load_dags("Instances/dags_300.csv");
    cout << "DAGs cargados: " << dags.size() << "\n";

    vector<TableImportance> optional_weights = load_optional_weights("Instances/important_tables.csv");
    cout << "Peso extra cargado para: " << optional_weights.size() << " tablas\n";

    // Parametros Tabu Search y tamanio maximo de dominio (capacidad)
    const int MAX_CAPACITY = 50;
    const int MAX_ITER     = 1000;
    const int TABU_TENURE  = 10;

    // Solucion Inicial (Greedy)
    Solution initial_sol = run_greedy(dags, optional_weights, MAX_CAPACITY);
    cout << "Greedy: " << initial_sol.clusters.size() << " dominios generados\n";

    // Validar que el Greedy produjo una solución factible
    if (!is_feasible_solution(initial_sol, dags, MAX_CAPACITY))
        cerr << "ERROR: solucion inicial del greedy es infactible\n";


    // Mejorar solucion con Búsqueda Tabú
    Solution best_sol = run_tabu_search(initial_sol, dags, optional_weights, MAX_CAPACITY, MAX_ITER, TABU_TENURE);

    // Validar que el Tabu Search no produjo una solución infactible
    if (!is_feasible_solution(best_sol, dags, MAX_CAPACITY))
        cerr << "ERROR: solucion final del tabu search es infactible\n";

    // Recalcular con sobrecarga no-const para actualizar write_conflicts por clúster
    best_sol.fitness_score = evaluate_solution(best_sol, dags, optional_weights);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    // Reporting
    auto critical_tables = build_critical_table_ranking(dags);
    auto cluster_reports = build_cluster_reports(best_sol, dags);

    print_summary(best_sol, dags, cluster_reports, critical_tables, 15);

    cout << "Tiempo de ejecucion: " << duration.count() << " segundos\n\n";


    save_solution_to_csv(best_sol, dags,     "Solved/dags_300_solved.csv");
    save_critical_tables_csv(critical_tables, "Solved/critical_tables.csv");
    save_cluster_report_csv(cluster_reports,  "Solved/cluster_report.csv");

    cout << "Archivos exportados en Solved/\n";
    cout << "  - dags_300_solved.csv  -> asignacion DAG -> dominio\n";
    cout << "  - critical_tables.csv  -> ranking de tablas criticas\n";
    cout << "  - cluster_report.csv   -> score y semaforo por dominio\n";

    return 0;
}