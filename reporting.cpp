#include "reporting.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>

extern int calculate_affinity(const DAG& dag1, const DAG& dag2);

// ---------------------------------------------------------------------------
// build_critical_table_ranking
// Para cada tabla del ecosistema calcula:
//   - frequency : en cuántos DAGs aparece (source o target)
//   - as_source : cuántos DAGs la leen
//   - as_target : cuántos DAGs escriben en ella
//   - is_contested : true si más de un DAG escribe en ella (riesgo de conflicto)
// ---------------------------------------------------------------------------
std::vector<CriticalTable> build_critical_table_ranking(const std::vector<DAG>& dags) {
    // tabla -> {freq, as_source, as_target}
    std::unordered_map<std::string, CriticalTable> table_map;

    for (const auto& dag : dags) {
        // Usar un set por DAG para no contar la misma tabla dos veces dentro del mismo DAG
        std::unordered_set<std::string> seen_in_dag;

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
            // as_target puede contar múltiples DAGs escribiendo la misma tabla
            table_map[t].as_target++;
        }
    }

    // Marcar tablas en disputa (más de un DAG escribe en ellas)
    for (auto& [name, ct] : table_map) {
        ct.is_contested = ct.as_target > 1;
    }

    // Convertir a vector y ordenar por frecuencia descendente
    std::vector<CriticalTable> result;
    result.reserve(table_map.size());
    for (auto& [name, ct] : table_map) result.push_back(ct);

    std::sort(result.begin(), result.end(), [](const CriticalTable& a, const CriticalTable& b) {
        return a.frequency > b.frequency;
    });

    return result;
}

// ---------------------------------------------------------------------------
// build_cluster_reports
// Para cada clúster calcula:
//   - Afinidad interna total
//   - Pares con relación de linaje (productor→consumidor)
//   - Conflictos de escritura
//   - Score de cohesión normalizado 0-100
//   - Semáforo: GREEN / YELLOW / RED
// ---------------------------------------------------------------------------
std::vector<ClusterReport> build_cluster_reports(const Solution& sol, const std::vector<DAG>& dags) {
    std::vector<ClusterReport> reports;

    for (const auto& cluster : sol.clusters) {
        ClusterReport r;
        r.cluster_id    = cluster.id;
        r.num_dags      = cluster.dags_indices.size();
        r.capacity_used = cluster.current_capacity;
        r.internal_affinity  = 0;
        r.write_conflicts    = 0;
        r.lineage_pairs      = 0;

        // Recopilar tablas únicas del clúster
        std::unordered_set<std::string> unique_t;
        for (int idx : cluster.dags_indices) {
            for (const auto& t : dags[idx].source_tables) unique_t.insert(t);
            for (const auto& t : dags[idx].target_tables) unique_t.insert(t);
        }
        r.unique_tables = std::vector<std::string>(unique_t.begin(), unique_t.end());

        // Detectar conflictos de escritura
        std::unordered_map<std::string, int> target_writers;
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

                // Detectar linaje: ¿alguna target del i es source del j o viceversa?
                const auto& ti = dags[cluster.dags_indices[i]];
                const auto& tj = dags[cluster.dags_indices[j]];
                std::unordered_set<std::string> targets_i(ti.target_tables.begin(), ti.target_tables.end());
                std::unordered_set<std::string> targets_j(tj.target_tables.begin(), tj.target_tables.end());
                for (const auto& t : tj.source_tables)
                    if (targets_i.count(t)) { r.lineage_pairs++; break; }
                for (const auto& t : ti.source_tables)
                    if (targets_j.count(t)) { r.lineage_pairs++; break; }
            }
        }

        // Score de cohesión (0-100)
        // Fórmula: parte positiva basada en afinidad/pares, penalización por conflictos
        if (total_pairs == 0) {
            r.cohesion_score = 50.0; // Clúster de un solo DAG: neutro
        } else {
            // Normalizar afinidad: afinidad promedio por par, escalada
            // El máximo teórico por par con los pesos actuales sería ~10 tablas * WEIGHT_LINEAGE = 30
            const double MAX_AFF_PER_PAIR = 10.0;
            double aff_normalized = std::min(100.0, (r.internal_affinity / (double)total_pairs) / MAX_AFF_PER_PAIR * 100.0);

            // Penalización por conflictos: cada conflicto resta 10 puntos
            double conflict_penalty = std::min(100.0, r.write_conflicts * 10.0);

            r.cohesion_score = std::max(0.0, aff_normalized - conflict_penalty);
        }

        // Semáforo
        if (r.cohesion_score >= 65.0)      r.semaphore = "GREEN";
        else if (r.cohesion_score >= 35.0) r.semaphore = "YELLOW";
        else                                r.semaphore = "RED";

        reports.push_back(r);
    }

    return reports;
}

// ---------------------------------------------------------------------------
// print_summary: salida en consola
// ---------------------------------------------------------------------------
void print_summary(const Solution& sol,
                   const std::vector<DAG>& dags,
                   const std::vector<ClusterReport>& reports,
                   const std::vector<CriticalTable>& critical_tables,
                   int top_n_tables) {

    std::cout << "\n========================================================\n";
    std::cout << "  REPORTE DE DOMINIOS DE DATOS\n";
    std::cout << "========================================================\n";
    std::cout << "  Total DAGs      : " << dags.size() << "\n";
    std::cout << "  Total Dominios  : " << sol.clusters.size() << "\n";
    std::cout << "  Fitness Global  : " << sol.fitness_score << "\n";
    std::cout << "--------------------------------------------------------\n";

    // Resumen por clúster
    std::cout << "\n  [ DOMINIOS DE DATOS ]\n\n";
    for (const auto& r : reports) {
        std::string icon = (r.semaphore == "GREEN") ? "[🟢]" :
                           (r.semaphore == "YELLOW") ? "[🟡]" : "[🔴]";
        std::cout << "  Dominio " << std::setw(3) << r.cluster_id
                  << " " << icon
                  << "  DAGs: " << std::setw(3) << r.num_dags
                  << "  I/O: "  << std::setw(3) << r.capacity_used
                  << "  Afinidad: " << std::setw(5) << r.internal_affinity
                  << "  Linaje: " << std::setw(3) << r.lineage_pairs
                  << "  Conflictos: " << std::setw(2) << r.write_conflicts
                  << "  Score: " << std::fixed << std::setprecision(1) << r.cohesion_score << "\n";
    }

    // Ranking de tablas críticas
    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "  [ TOP " << top_n_tables << " TABLAS CRÍTICAS ]\n\n";
    std::cout << "  " << std::left << std::setw(40) << "Tabla"
              << std::setw(8) << "Freq"
              << std::setw(10) << "Lecturas"
              << std::setw(10) << "Escrituras"
              << "Riesgo\n";
    std::cout << "  " << std::string(75, '-') << "\n";

    int shown = 0;
    for (const auto& ct : critical_tables) {
        if (shown >= top_n_tables) break;
        std::string risk = ct.is_contested ? " ⚠ DISPUTADA" : "";
        std::cout << "  " << std::left  << std::setw(40) << ct.name
                  << std::setw(8)  << ct.frequency
                  << std::setw(10) << ct.as_source
                  << std::setw(10) << ct.as_target
                  << risk << "\n";
        shown++;
    }
    std::cout << "\n========================================================\n\n";
}

// ---------------------------------------------------------------------------
// Exportaciones CSV
// ---------------------------------------------------------------------------
void save_critical_tables_csv(const std::vector<CriticalTable>& tables, const std::string& filename) {
    std::ofstream f(filename);
    f << "rank,table_name,frequency,as_source,as_target,is_contested\n";
    int rank = 1;
    for (const auto& ct : tables) {
        f << rank++ << "," << ct.name << "," << ct.frequency << ","
          << ct.as_source << "," << ct.as_target << ","
          << (ct.is_contested ? "true" : "false") << "\n";
    }
    f.close();
}

void save_cluster_report_csv(const std::vector<ClusterReport>& reports, const std::string& filename) {
    std::ofstream f(filename);
    f << "cluster_id,num_dags,capacity_used,internal_affinity,lineage_pairs,write_conflicts,cohesion_score,semaphore,unique_tables_count\n";
    for (const auto& r : reports) {
        f << r.cluster_id << "," << r.num_dags << "," << r.capacity_used << ","
          << r.internal_affinity << "," << r.lineage_pairs << "," << r.write_conflicts << ","
          << std::fixed << std::setprecision(2) << r.cohesion_score << ","
          << r.semaphore << "," << r.unique_tables.size() << "\n";
    }
    f.close();
}