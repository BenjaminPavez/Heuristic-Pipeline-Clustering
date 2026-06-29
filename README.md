# Heuristic-Pipeline-Clustering

> **A Heuristic Approach for the Logical Clustering of Data Pipelines**

This repository contains the implementation developed for my undergraduate thesis, which proposes a heuristic approach for the **Logical Clustering of Data Pipelines**. The problem is formulated as an adaptation of the **Topic-Based Conference Scheduling Problem (TBCSP)** and is solved using metaheuristic optimization techniques.

---

## Technical Specifications

| Component | Version |
|-----------|---------|
| Operating System | Ubuntu 24.04.4 LTS / Ubuntu 22.04.5 LTS |
| Compiler | G++ 13.3.0 |
| Language | C++ |

---

## Implemented Algorithms

The solution consists of two main stages:

1. **Greedy Algorithm**
   - Generates an initial feasible solution.

2. **Tabu Search**
   - Improves the initial solution using a best-improvement neighborhood exploration strategy.

---

## Project Structure

```text
.
├── Instances/
│   ├── dags_10.csv
│   ├── dags_20.csv
│   └── dags_100.csv
├── Solved/
│   ├── cluster_report.csv
│   ├── critical_tables.csv
│   └── dags_100_solved.csv
├── constraints.cpp
├── evaluation.cpp
├── greedy.cpp
├── greedy.h
├── main.cpp
├── reporting.cpp
├── reporting.h
├── tabu_search.cpp
├── tabu_search.h
├── types.h
├── utils.cpp
├── utils.h
├── Makefile
└── README.md
````

---

## Building the Project

Compile the project using the provided Makefile:

```bash
make
```

To run the executable:

```bash
make run
```

---

## Input

The input instances are located in the `Instances/` directory. Each CSV file represents a different problem instance.

---

## Output

The generated solutions are stored in the `Solved/` directory, including:

* `cluster_report.csv`: Summary of the generated clusters.
* `critical_tables.csv`: Critical tables identified during optimization.
* `dags_100_solved.csv`: Example of a solved instance.

---

## Thesis

This implementation accompanies the undergraduate thesis:

**"Acercamiento heurístico para el agrupamiento lógico de pipelines de datos"**

The proposed methodology adapts the **Topic-Based Conference Scheduling Problem (TBCSP)** to address the logical clustering of data pipelines through heuristic optimization.

