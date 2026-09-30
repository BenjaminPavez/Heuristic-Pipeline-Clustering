#include "tabu_search.h"
#include <vector>
#include <algorithm>
#include <climits>
#include <iostream>
#include <random>

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
La funcion perturba una solucion mediante la desintegracion del cluster con menor afinidad promedio de la solucion.
Los DAGs del cluster son repartidos (en orden aleatorio) al cluster factible con el que tengan mayor afinidad,
incluyendo clusters vacios (afinidad 0), por lo que un DAG puede quedar solo si no le conviene ningun otro cluster.

Parametros :
   const Solution& sol : Estructura solution con la solucion a perturbar.
   const vector<DAG>& dags : Vector con los DAGs extraidos del archivo de entrada.
   const vector<vector<int>>& M : Matriz de afinidad precalculada entre DAGs.
   int max_capacity : Entero con la capacidad maxima de los cluster.
   mt19937& rng : Generador de numeros aleatorios.

Retorno :
   Solution : Solucion con la perturbacion aplicada.

*/
static Solution perturb_solution(const Solution& sol, const vector<DAG>& dags, const vector<vector<int>>& M, int max_capacity, mt19937& rng) {
    Solution perturbed = sol;

    // Candidatos: clusters con al menos 2 DAGs, ordenados por afinidad promedio por par (peor primero)
    vector<pair<double,int>> candidates;
    for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
        const auto& idx = perturbed.clusters[c].dags_indices;
        if (idx.size() < 2) continue;
        long long aff_sum = 0; int pairs = 0;
        for (size_t i = 0; i < idx.size(); ++i)
            for (size_t j = i + 1; j < idx.size(); ++j) { aff_sum += M[idx[i]][idx[j]]; pairs++; }
        candidates.push_back({(double)aff_sum / pairs, (int)c});
    }
    if (candidates.empty()) return perturbed;
    sort(candidates.begin(), candidates.end());

    // Se elige al azar uno de los 3 peores clusters para no perturbar siempre el mismo
    int pick = uniform_int_distribution<int>(0, min<int>(3, candidates.size()) - 1)(rng);
    int worst_cluster_idx = candidates[pick].second;

    vector<int> dags_to_redistribute = perturbed.clusters[worst_cluster_idx].dags_indices;
    shuffle(dags_to_redistribute.begin(), dags_to_redistribute.end(), rng);
    perturbed.clusters[worst_cluster_idx].dags_indices.clear();
    perturbed.clusters[worst_cluster_idx].current_capacity = 0;

    for (int dag_id : dags_to_redistribute) {
        int best_target = -1;
        int best_aff = INT_MIN;

        for (size_t c = 0; c < perturbed.clusters.size(); ++c) {
            if ((int)c == worst_cluster_idx) continue;
            if (!is_feasible_move(perturbed.clusters[c], dags[dag_id], dags, max_capacity)) continue;

            int aff = 0;
            for (int idx : perturbed.clusters[c].dags_indices) aff += M[dag_id][idx];
            if (aff > best_aff) { best_aff = aff; best_target = (int)c; }
        }
        if (best_target == -1) best_target = worst_cluster_idx;   // sin destino factible: vuelve a su cluster

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

Oscilacion estrategica: durante la busqueda se permite que un dominio supere temporalmente la capacidad maxima,
penalizando el exceso con un factor P que se ajusta solo (sube si la busqueda pasa mucho tiempo infactible y baja
si pasa mucho tiempo factible). Esto permite cruzar entre soluciones factibles que con reubicaciones e intercambios
simples no estan conectadas cuando los dominios estan casi llenos. La mejor solucion global SOLO se actualiza con
soluciones factibles, por lo que el resultado final siempre cumple la capacidad maxima (mismo problema que Gurobi).

Parametros :
   const Solution& initial_sol : Estructura Solution con la solucion inicial entregada por Greedy.
   const vector<DAG>& dags : Vector de estructura DAG con la informacion de todos los DAGs.
   const vector<TableImportance>& optional_weights : Vector de estructura TableImportance con la importancia opcional de las tablas.
   int max_capacity : Entero con la capacidad maxima de los cluster.
   int max_iterations : Entero con la cantidad maxima de iteraciones.
   int tabu_tenure : Entero con la cantidad maxima de almacenamiento de los movimientos.
   unsigned seed : Semilla del generador aleatorio (desempates, tenencia y perturbacion).

Retorno :
   Solution : Solucion final obtenida con la aplicacion de tabu search.

*/
Solution run_tabu_search(const Solution& initial_sol, const vector<DAG>& dags, const vector<TableImportance>& optional_weights, int max_capacity, int max_iterations, int tabu_tenure, unsigned seed) {
    const size_t n = dags.size();

    // Afinidad precalculada
    vector<vector<int>> M(n, vector<int>(n, 0));
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            M[i][j] = M[j][i] = calculate_affinity(dags[i], dags[j], optional_weights);

    vector<int> w(n);
    for (size_t i = 0; i < n; ++i) w[i] = get_dag_weight(dags[i]);

    mt19937 rng(seed);

    Solution current_sol = initial_sol;
    current_sol.fitness_score = evaluate_solution(current_sol, dags, optional_weights);
    Solution best_global_sol = current_sol;

    const size_t K = current_sol.clusters.size();
    vector<vector<int>> S;
    build_cluster_affinity(current_sol, M, S);

    vector<vector<int>> tabu_matrix(n, vector<int>(K, 0));

    // Perturbar si no mejora en 5% de las iteraciones
    const int STAGNATION_LIMIT = max(100, max_iterations / 20);

    // Tenencia adaptada al tamanio: en instancias pequenias una tenencia de 20 bloquea casi todos los movimientos
    const int base_tenure = max(3, min(tabu_tenure, (int)n / 3));
    uniform_int_distribution<int> tenure_dist(base_tenure, base_tenure + max(1, base_tenure / 2));
    int stagnation_counter = 0;

    // ---- Oscilacion estrategica ----
    // Exceso de capacidad de un dominio (0 si cumple C_max)
    auto excess = [&](int cap) { return cap > max_capacity ? cap - max_capacity : 0; };
    // Limite duro del exceso por dominio, para no alejarse demasiado de la region factible
    int max_w = 0;
    for (size_t i = 0; i < n; ++i) max_w = max(max_w, w[i]);
    const int MAX_OVER = max_capacity + max_w;
    long long penalty = 2;                  // P: costo por cada unidad de exceso de capacidad
    const long long PENALTY_MIN = 1, PENALTY_MAX = 1LL << 20;
    const int PENALTY_WINDOW = 10;          // cada 10 iteraciones se ajusta P
    int feasible_in_window = 0;
    int total_excess = 0;                   // suma del exceso de todos los dominios de la solucion actual

    for (int iter = 1; iter <= max_iterations; ++iter) {

        long long best_val = LLONG_MIN;     // fitness penalizado del mejor movimiento
        int best_fit = 0, best_excess = 0;
        int mv_type = -1;               // 0 = reubicacion, 1 = intercambio

        // DAG que estaba inicialmente
        int mv_a = -1, mv_A = -1, mv_ia = -1;

        // DAG a reubicar
        int mv_b = -1, mv_B = -1, mv_ib = -1;

        // Empates entre movimientos con el mismo valor se resuelven al azar (muestreo de reservorio)
        int ties = 0;

        auto consider = [&](int fit, int new_excess, bool is_tabu) -> bool {
            // Aspiracion: el movimiento lleva a una solucion factible mejor que la mejor global
            bool aspiration = new_excess == 0 && fit > best_global_sol.fitness_score;
            if (is_tabu && !aspiration) return false;
            long long val = (long long)fit - penalty * new_excess;
            if (val > best_val) ties = 1;
            else if (val < best_val || rng() % (++ties) != 0) return false;
            best_val = val; best_fit = fit; best_excess = new_excess;
            return true;
        };

        // Reubicar un DAG en otro dominio
        for (size_t A = 0; A < K; ++A) {
            const auto& CA = current_sol.clusters[A].dags_indices;
            int capA = current_sol.clusters[A].current_capacity;
            for (size_t ia = 0; ia < CA.size(); ++ia) {
                int d = CA[ia];
                int dExA = excess(capA - w[d]) - excess(capA);
                for (size_t B = 0; B < K; ++B) {
                    if (A == B) continue;
                    int capB = current_sol.clusters[B].current_capacity;
                    if (capB + w[d] > MAX_OVER) continue;

                    int fit = current_sol.fitness_score + S[d][B] - S[d][A];
                    int new_excess = total_excess + dExA + excess(capB + w[d]) - excess(capB);
                    if (consider(fit, new_excess, tabu_matrix[d][B] >= iter)) {
                        mv_type = 0;
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
                        int newA = capA - w[a] + w[b], newB = capB - w[b] + w[a];
                        if (newA > MAX_OVER || newB > MAX_OVER) continue;

                        int delta = (S[a][B] - M[a][b]) - S[a][A] + (S[b][A] - M[b][a]) - S[b][B];
                        int fit = current_sol.fitness_score + delta;
                        int new_excess = total_excess + excess(newA) - excess(capA) + excess(newB) - excess(capB);
                        if (consider(fit, new_excess, tabu_matrix[a][B] >= iter || tabu_matrix[b][A] >= iter)) {
                            mv_type = 1;
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
            tabu_matrix[mv_a][mv_A] = iter + tenure_dist(rng);

        } else {
            CA.dags_indices[mv_ia] = mv_b;
            CB.dags_indices[mv_ib] = mv_a;
            CA.current_capacity += w[mv_b] - w[mv_a];
            CB.current_capacity += w[mv_a] - w[mv_b];
            for (size_t x = 0; x < n; ++x) {
                S[x][mv_A] += M[x][mv_b] - M[x][mv_a];
                S[x][mv_B] += M[x][mv_a] - M[x][mv_b];
            }
            tabu_matrix[mv_a][mv_A] = iter + tenure_dist(rng);
            tabu_matrix[mv_b][mv_B] = iter + tenure_dist(rng);
        }
        current_sol.fitness_score = best_fit;
        total_excess = best_excess;

        // Solo una solucion factible puede ser la mejor global
        if (total_excess == 0 && current_sol.fitness_score > best_global_sol.fitness_score) {
            best_global_sol = current_sol;
            stagnation_counter = 0;
        } else {
            stagnation_counter++;
        }

        // Ajuste de la penalizacion: si toda la ventana fue infactible se duplica, si toda fue factible se reduce a la mitad
        if (total_excess == 0) feasible_in_window++;
        if (iter % PENALTY_WINDOW == 0) {
            if (feasible_in_window == 0)                    penalty = min(PENALTY_MAX, penalty * 2);
            else if (feasible_in_window == PENALTY_WINDOW)  penalty = max(PENALTY_MIN, penalty / 2);
            feasible_in_window = 0;
        }

        // Diversificacion: si llevamos STAGNATION_LIMIT iteraciones sin mejorar, perturbar
        if (stagnation_counter >= STAGNATION_LIMIT) {
            cout << "  [Tabu] Perturbando en iteracion " << iter << " (estancamiento)\n";
            // Si la solucion actual no es factible, se perturba la mejor solucion encontrada
            const Solution& base = (total_excess == 0) ? current_sol : best_global_sol;
            current_sol = perturb_solution(base, dags, M, max_capacity, rng);
            current_sol.fitness_score = evaluate_solution(current_sol, dags, optional_weights);
            build_cluster_affinity(current_sol, M, S);
            total_excess = 0;
            for (const auto& c : current_sol.clusters) total_excess += excess(c.current_capacity);
            stagnation_counter = 0;
            for (auto& row : tabu_matrix)
                fill(row.begin(), row.end(), 0);
        }
    }

    return best_global_sol;
}