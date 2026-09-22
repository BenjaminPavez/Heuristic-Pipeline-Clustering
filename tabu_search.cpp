#include "tabu_search.h"
#include <vector>
#include <algorithm>
#include <climits>
#include <iostream>

using namespace std;


// Funciones externas
extern int evaluate_solution(const Solution& sol, const vector<DAG>& all_dags, const vector<TableImportance>& optional_weights);
extern bool is_feasible_move(const Cluster& target_cluster, const DAG& dag_to_move, const vector<DAG>& all_dags, int max_capacity);
extern int calculate_affinity(const DAG& dag1, const DAG& dag2, const vector<TableImportance>& optional_weights);



/*
La funcion calcula el peso del dag sumando sus tablas de origen con la cantidad de tablas de destino.

Parametros :
   const DAG& dag : Estructura DAG con la informacion de un DAG en particular.

Retorno :
   int : Solucion con la perturbacion aplicada.

*/
int get_dag_weight(const DAG& dag) {
    return dag.num_source_tables + dag.num_target_tables;
}



/*
La funcion perturba una solucion mediante la desintegracion del cluster con menor afinidad de la solucion, los DAGs
del cluster son repartidos a otros clusters que tengan el espacio y que cumplan con las restricciones.

Parametros :
   const Solution& sol : Estructura solution con la solucion a perturbar.
   const vector<DAG>& dags : Vector con los DAGs extraidos del archivo de entrada.
   int max_capacity : Entero con la capacidad maxima de los cluster.

Retorno :
   Solution : Solucion con la perturbacion aplicada.

*/
static Solution perturb_solution(const Solution& sol, const vector<DAG>& dags, int max_capacity) {
    Solution perturbed = sol;

    int worst_cluster_idx = 0;
    double worst_cohesion = 1e9;

    for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
        const auto& cluster = perturbed.clusters[c];
        if (cluster.dags_indices.size() < 2) continue;

        int pairs = 0, aff_sum = 0;
        for (size_t i = 0; i < cluster.dags_indices.size(); ++i)
            for (size_t j = i + 1; j < cluster.dags_indices.size(); ++j) {
                aff_sum++; pairs++;
            }
        double cohesion = pairs > 0 ? (double)aff_sum / pairs : 0.0;
        if (cohesion < worst_cohesion) {
            worst_cohesion = cohesion;
            worst_cluster_idx = c;
        }
    }

    vector<int> dags_to_redistribute = perturbed.clusters[worst_cluster_idx].dags_indices;
    perturbed.clusters[worst_cluster_idx].dags_indices.clear();
    perturbed.clusters[worst_cluster_idx].current_capacity = 0;

    for (int dag_id : dags_to_redistribute) {
        int best_target = worst_cluster_idx;
        int best_fit = -1;

        for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
            if ((int)c == worst_cluster_idx) continue;
            if (!is_feasible_move(perturbed.clusters[c], dags[dag_id], dags, max_capacity)) continue;

            int aff = 0;
            for (int idx [[maybe_unused]] : perturbed.clusters[c].dags_indices)
                aff++;
            if (aff > best_fit) { best_fit = aff; best_target = c; }
        }

        perturbed.clusters[best_target].dags_indices.push_back(dag_id);
        perturbed.clusters[best_target].current_capacity += get_dag_weight(dags[dag_id]);
    }

    return perturbed;
}



/*
La funcion rellena la matriz S[d][k] que guarda la afinidad total del DAG d con los DAGs del dominio k.

Parametros :
   const Solution& sol : Estructura Solution con la solucion inicial entregada por Greedy.
   const vector<vector<int>>& M : Matriz que almacena la afinidad que existe entre los DAGs.
   vector<vector<int>>& S : Matriz que almacena la afinidad total del DAG d con los DAGs del dominio k.

Retorno :
   Al ser una funcion void no retorna nada.

*/
static void build_cluster_affinity(const Solution& sol, const vector<vector<int>>& M, vector<vector<int>>& S) {
    size_t n = M.size(), K = sol.clusters.size();
    S.assign(n, vector<int>(K, 0));
    for (size_t k = 0; k < K; ++k)
        for (int x : sol.clusters[k].dags_indices)
            for (size_t d = 0; d < n; ++d)
                S[d][k] += M[d][x];
}



/*
La funcion ejecuta la metaheuristica Tabu Search para mejorar la solucion inicial.

Parametros :
   const Solution& initial_sol : Estructura Solution con la solucion inicial entregada por Greedy.
   const vector<DAG>& dags : Vector de estructura DAG con la informacion de todos los DAGs.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con la importancia opcional de las tablas.
   int max_capacity : Entero con la capacidad maxima de los cluster.
   int max_iterations : Entero con la cantidad maxima de iteraciones.
   int tabu_tenure : Entero con la cantidad maxima de almacenamiento de los movimientos.

Retorno :
   Solution : Solucion final obtenida con la aplicacion de tabu search.

*/
Solution run_tabu_search(const Solution& initial_sol, const vector<DAG>& dags, const vector<TableImportance>& optional_weights, int max_capacity, int max_iterations, int tabu_tenure) {
    const size_t n = dags.size();

    // Afinidad precalculada
    vector<vector<int>> M(n, vector<int>(n, 0));
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            M[i][j] = M[j][i] = calculate_affinity(dags[i], dags[j], optional_weights);

    vector<int> w(n);
    for (size_t i = 0; i < n; ++i) w[i] = get_dag_weight(dags[i]);

    Solution current_sol = initial_sol;
    current_sol.fitness_score = evaluate_solution(current_sol, dags, optional_weights);
    Solution best_global_sol = current_sol;

    const size_t K = current_sol.clusters.size();
    vector<vector<int>> S;
    build_cluster_affinity(current_sol, M, S);

    vector<vector<int>> tabu_matrix(n, vector<int>(K, 0));

    const int STAGNATION_LIMIT = max_iterations / 5; // Perturbar si no mejora en 20% de las iteraciones
    int stagnation_counter = 0;

    for (int iter = 1; iter <= max_iterations; ++iter) {

        int best_fit = INT_MIN;
        int mv_type = -1;               // 0 = reubicacion, 1 = intercambio

        // DAG que estaba inicialmente
        int mv_a = -1, mv_A = -1, mv_ia = -1;

        // DAG a reubicar
        int mv_b = -1, mv_B = -1, mv_ib = -1;

        // Reubicar un DAG en otro dominio con capacidad
        for (size_t A = 0; A < K; ++A) {
            const auto& CA = current_sol.clusters[A].dags_indices;
            for (size_t ia = 0; ia < CA.size(); ++ia) {
                int d = CA[ia];
                for (size_t B = 0; B < K; ++B) {
                    if (A == B) continue;
                    if (current_sol.clusters[B].current_capacity + w[d] > max_capacity) continue;

                    int fit = current_sol.fitness_score + S[d][B] - S[d][A];
                    bool is_tabu    = tabu_matrix[d][B] >= iter;
                    bool aspiration = fit > best_global_sol.fitness_score;
                    if ((!is_tabu || aspiration) && fit > best_fit) {
                        best_fit = fit; mv_type = 0;
                        mv_a = d; mv_A = A; mv_ia = ia; mv_B = B;
                    }
                }
            }
        }

        // Intercambiar dos DAGs de clusters distintos
        for (size_t A = 0; A < K; ++A) {
            const auto& CA = current_sol.clusters[A].dags_indices;
            int capA = current_sol.clusters[A].current_capacity;
            for (size_t B = A + 1; B < K; ++B) {
                const auto& CB = current_sol.clusters[B].dags_indices;
                int capB = current_sol.clusters[B].current_capacity;
                for (size_t ia = 0; ia < CA.size(); ++ia) {
                    int a = CA[ia];
                    for (size_t ib = 0; ib < CB.size(); ++ib) {
                        int b = CB[ib];
                        if (capA - w[a] + w[b] > max_capacity) continue;
                        if (capB - w[b] + w[a] > max_capacity) continue;

                        int delta = (S[a][B] - M[a][b]) - S[a][A] + (S[b][A] - M[b][a]) - S[b][B];
                        int fit = current_sol.fitness_score + delta;
                        bool is_tabu    = tabu_matrix[a][B] >= iter || tabu_matrix[b][A] >= iter;
                        bool aspiration = fit > best_global_sol.fitness_score;
                        if ((!is_tabu || aspiration) && fit > best_fit) {
                            best_fit = fit; mv_type = 1;
                            mv_a = a; mv_A = A; mv_ia = ia;
                            mv_b = b; mv_B = B; mv_ib = ib;
                        }
                    }
                }
            }
        }

        if (mv_type == -1) break;

        // Aplicar el movimiento y actualizar S
        auto& CA = current_sol.clusters[mv_A];
        auto& CB = current_sol.clusters[mv_B];
        if (mv_type == 0) {
            CA.dags_indices.erase(CA.dags_indices.begin() + mv_ia);
            CB.dags_indices.push_back(mv_a);
            CA.current_capacity -= w[mv_a];
            CB.current_capacity += w[mv_a];
            for (size_t x = 0; x < n; ++x) {
                S[x][mv_A] -= M[x][mv_a];
                S[x][mv_B] += M[x][mv_a];
            }
            tabu_matrix[mv_a][mv_A] = iter + tabu_tenure;

        } else {
            CA.dags_indices[mv_ia] = mv_b;
            CB.dags_indices[mv_ib] = mv_a;
            CA.current_capacity += w[mv_b] - w[mv_a];
            CB.current_capacity += w[mv_a] - w[mv_b];
            for (size_t x = 0; x < n; ++x) {
                S[x][mv_A] += M[x][mv_b] - M[x][mv_a];
                S[x][mv_B] += M[x][mv_a] - M[x][mv_b];
            }
            tabu_matrix[mv_a][mv_A] = iter + tabu_tenure;
            tabu_matrix[mv_b][mv_B] = iter + tabu_tenure;
        }
        current_sol.fitness_score = best_fit;

        if (current_sol.fitness_score > best_global_sol.fitness_score) {
            best_global_sol    = current_sol;
            stagnation_counter = 0;
        } else {
            stagnation_counter++;
        }

        // Diversificacion: si llevamos STAGNATION_LIMIT iteraciones sin mejorar, perturbar
        if (stagnation_counter >= STAGNATION_LIMIT) {
            cout << "  [Tabu] Perturbando en iteracion " << iter << " (estancamiento)\n";
            current_sol = perturb_solution(current_sol, dags, max_capacity);
            current_sol.fitness_score = evaluate_solution(current_sol, dags, optional_weights);
            build_cluster_affinity(current_sol, M, S);
            stagnation_counter = 0;
            for (auto& row : tabu_matrix)
                fill(row.begin(), row.end(), 0);
        }
    }

    return best_global_sol;
}