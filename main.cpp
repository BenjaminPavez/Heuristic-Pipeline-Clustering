#include <iostream>
#include <chrono>
#include "types.h"
#include "utils.h"       // Para load_dags y save_solution_to_csv
#include "greedy.h"      // Para run_greedy
#include "tabu_search.h" // Para run_tabu_search

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    // 1. Cargar datos 
    // Asegúrate de que el archivo CSV de entrada exista en la misma carpeta
    std::vector<DAG> dags = load_dags("instancia_corporativa.csv");
    
    // 2. Solución Constructiva
    // Parámetro 50 es un ejemplo de la capacidad máxima de tablas por clúster (I/O)
    Solution initial_sol = run_greedy(dags, 50);
    
    // 3. Mejoramiento Heurístico
    // Parámetros: max_capacity=50, max_iterations=1000, tabu_tenure=10
    Solution best_sol = run_tabu_search(initial_sol, dags, 50, 1000, 10);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Resultados finales impresos en consola
    std::cout << "====================================\n";
    std::cout << "Agrupamiento Logico Finalizado\n";
    std::cout << "====================================\n";
    std::cout << "Mejor Fitness (Afinidad): " << best_sol.fitness_score << "\n";
    std::cout << "Tiempo de ejecucion: " << duration.count() << " segundos\n";

    // Exportar el resultado final para que Airflow lo lea
    save_solution_to_csv(best_sol, dags, "dominios_logicos.csv");

    return 0;
}