#include "reporting.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;


// Funciones externas
extern int calculate_affinity(const DAG& dag1, const DAG& dag2);



/*
La funcion construye el ranking de las tablas.

Parametros :
   const vector<DAG>& dags : Vector de estructura DAG que contiene todos los DAGs.

Retorno :
   vector<CriticalTable> : Vector con las tablas criticas ordenadas.

*/
vector<CriticalTable> build_critical_table_ranking(const vector<DAG>& dags) {
    // tabla -> {freq, as_source, as_target}
    unordered_map<string, CriticalTable> table_map;

    for (const auto& dag : dags) {
        // Usar un set por DAG para no contar la misma tabla dos veces dentro del mismo DAG
        unordered_set<string> seen_in_dag;

        for (const auto& t : dag.source_tables) {
            if (!seen_in_dag.count(t)) {
                table_map[t].name = t;
                table_map[t].frequency++;
                table_map[t].as_source++;
                seen_in_dag.insert(t);
            }
        }
        for (const auto& t : dag.target_tables) {
            if (!seen_in_dag.count(t)) {
                table_map[t].name = t;
                table_map[t].frequency++;
                seen_in_dag.insert(t);
            }
            // as_target puede contar multiples DAGs escribiendo la misma tabla
            table_map[t].as_target++;
        }
    }

    // Marcar tablas en disputa (mas de un DAG escribe en ellas)
    for (auto& [name, ct] : table_map) {
        ct.is_contested = ct.as_target > 1;
    }

    // Convertir a vector y ordenar por frecuencia descendente
    vector<CriticalTable> result;
    result.reserve(table_map.size());
    for (auto& [name, ct] : table_map) result.push_back(ct);

    sort(result.begin(), result.end(), [](const CriticalTable& a, const CriticalTable& b) {
        return a.frequency > b.frequency;
    });

    return result;
}



/*
La funcion construye el reporte de los clusters.

Parametros :
   const Solution& sol : Vector de estructura Solution que contiene la solucion final.
   const vector<DAG>& dags : Vector de estructura DAG que contiene todos los DAGs.

Retorno :
   vector<ClusterReport> : Vector con el reporte de los clusters.

*/
vector<ClusterReport> build_cluster_reports(const Solution& sol, const vector<DAG>& dags) {
    vector<ClusterReport> reports;

    for (const auto& cluster : sol.clusters) {
        ClusterReport r;
        r.cluster_id    = cluster.id;
        r.num_dags      = cluster.dags_indices.size();
        r.capacity_used = cluster.current_capacity;
        r.internal_affinity  = 0;
        r.write_conflicts    = 0;
        r.lineage_pairs      = 0;

        // Recopilar tablas unicas del cluster
        unordered_set<string> unique_t;
        for (int idx : cluster.dags_indices) {
            for (const auto& t : dags[idx].source_tables) unique_t.insert(t);
            for (const auto& t : dags[idx].target_tables) unique_t.insert(t);
        }
        r.unique_tables = vector<string>(unique_t.begin(), unique_t.end());

        // Detectar conflictos de escritura
        unordered_map<string, int> target_writers;
        for (int idx : cluster.dags_indices)
            for (const auto& t : dags[idx].target_tables)
                target_writers[t]++;
        for (const auto& [t, cnt] : target_writers)
            if (cnt > 1) r.write_conflicts += (cnt - 1);

        // Calcular afinidad interna par a par y contar relaciones de linaje
        int total_pairs = 0;
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i) {
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j) {
                int aff = calculate_affinity(dags[cluster.dags_indices[i]], dags[cluster.dags_indices[j]]);
                r.internal_affinity += aff;
                total_pairs++;

                // Detectar linaje
                const auto& ti = dags[cluster.dags_indices[i]];
                const auto& tj = dags[cluster.dags_indices[j]];
                unordered_set<string> targets_i(ti.target_tables.begin(), ti.target_tables.end());
                unordered_set<string> targets_j(tj.target_tables.begin(), tj.target_tables.end());
                for (const auto& t : tj.source_tables)
                    if (targets_i.count(t)) { r.lineage_pairs++; break; }
                for (const auto& t : ti.source_tables)
                    if (targets_j.count(t)) { r.lineage_pairs++; break; }
            }
        }

        if (total_pairs == 0) {
            r.cohesion_score = 50.0;
        } else {
            const double MAX_AFF_PER_PAIR = 10.0;
            double aff_normalized = min(100.0, (r.internal_affinity / (double)total_pairs) / MAX_AFF_PER_PAIR * 100.0);
            double conflict_penalty = min(100.0, r.write_conflicts * 10.0);
            r.cohesion_score = max(0.0, aff_normalized - conflict_penalty);
        }

        // Semaforo
        if (r.cohesion_score >= 65.0)      r.semaphore = "GREEN";
        else if (r.cohesion_score >= 35.0) r.semaphore = "YELLOW";
        else                                r.semaphore = "RED";

        reports.push_back(r);
    }

    return reports;
}



/*
La funcion imprime el resumen general por pantalla.

Parametros :
   const Solution& sol : Vector de estructura Solution que contiene la solucion final.
   const vector<DAG>& dags : Vector de estructura DAG que contiene todos los DAGs.
   const vector<ClusterReport>& reports : Vector de estructura ClusterReport que contiene el reporte de los clusters.
   const vector<CriticalTable>& critical_tables : Vector de estructura CriticalTable que contiene el reporte de las tablas criticas.
   int top_n_tables : Entero con el numero de las tablas importantes a mostrar por pantalla.

Retorno :
   Al ser una funcion void no retorna nada.

*/
void print_summary(const Solution& sol, const vector<DAG>& dags, const vector<ClusterReport>& reports, const vector<CriticalTable>& critical_tables, int top_n_tables) {

    cout << "\n========================================================\n";
    cout << "  REPORTE DE DOMINIOS DE DATOS\n";
    cout << "========================================================\n";
    cout << "  Total DAGs      : " << dags.size() << "\n";
    cout << "  Total Dominios  : " << sol.clusters.size() << "\n";
    cout << "  Fitness Global  : " << sol.fitness_score << "\n";
    cout << "--------------------------------------------------------\n";

    // Resumen por cluster
    cout << "\n  [ DOMINIOS DE DATOS ]\n\n";
    for (const auto& r : reports) {
        string icon = (r.semaphore == "GREEN") ? "[GREEN]" :
                           (r.semaphore == "YELLOW") ? "[YELLOW]" : "[RED]";
        cout << "  Dominio " << setw(3) << r.cluster_id
                  << " " << icon
                  << "  DAGs: " << setw(3) << r.num_dags
                  << "  I/O: "  << setw(3) << r.capacity_used
                  << "  Afinidad: " << setw(5) << r.internal_affinity
                  << "  Linaje: " << setw(3) << r.lineage_pairs
                  << "  Conflictos: " << setw(2) << r.write_conflicts
                  << "  Score: " << fixed << setprecision(1) << r.cohesion_score << "\n";
    }

    // Ranking de tablas criticas
    cout << "\n--------------------------------------------------------\n";
    cout << "  [ TOP " << top_n_tables << " TABLAS CRÍTICAS ]\n\n";
    cout << "  " << left << setw(40) << "Tabla"
              << setw(8) << "Freq"
              << setw(10) << "Lecturas"
              << setw(10) << "Escrituras"
              << "Riesgo\n";
    cout << "  " << string(75, '-') << "\n";

    int shown = 0;
    for (const auto& ct : critical_tables) {
        if (shown >= top_n_tables) break;
        string risk = ct.is_contested ? " DISPUTADA" : "";
        cout << "  " << left  << setw(40) << ct.name
                  << setw(8)  << ct.frequency
                  << setw(10) << ct.as_source
                  << setw(10) << ct.as_target
                  << risk << "\n";
        shown++;
    }
    cout << "\n========================================================\n\n";
}



/*
La funcion guarda las tablas criticas en un .csv.

Parametros :
   const vector<CriticalTable>& critical_tables : Vector de estructura CriticalTable que contiene el reporte de las tablas criticas.
   const string& filename : String con el nombre de archivo de salida.

Retorno :
   Al ser una funcion void no retorna nada.

*/
void save_critical_tables_csv(const vector<CriticalTable>& tables, const string& filename) {
    ofstream f(filename);
    f << "rank,table_name,frequency,as_source,as_target,is_contested\n";
    int rank = 1;
    for (const auto& ct : tables) {
        f << rank++ << "," << ct.name << "," << ct.frequency << ","
          << ct.as_source << "," << ct.as_target << ","
          << (ct.is_contested ? "true" : "false") << "\n";
    }
    f.close();
}



/*
La funcion guarda el reporte de los clusters en un .csv.

Parametros :
   const vector<ClusterReport>& reports : Vector de estructura ClusterReport que contiene el reporte de los clusters.
   const string& filename : String con el nombre de archivo de salida.

Retorno :
   Al ser una funcion void no retorna nada.

*/
void save_cluster_report_csv(const vector<ClusterReport>& reports, const string& filename) {
    ofstream f(filename);
    f << "cluster_id,num_dags,capacity_used,internal_affinity,lineage_pairs,write_conflicts,cohesion_score,semaphore,unique_tables_count\n";
    for (const auto& r : reports) {
        f << r.cluster_id << "," << r.num_dags << "," << r.capacity_used << ","
          << r.internal_affinity << "," << r.lineage_pairs << "," << r.write_conflicts << ","
          << fixed << setprecision(2) << r.cohesion_score << ","
          << r.semaphore << "," << r.unique_tables.size() << "\n";
    }
    f.close();
}