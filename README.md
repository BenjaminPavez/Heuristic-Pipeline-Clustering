# Heuristic-Pipeline-Clustering

> [!NOTE]  
> The latest version is available in the `feat-test-gurobi-v2` branch.

> **A Heuristic Approach for the Logical Clustering of Data Pipelines**

This repository contains the implementation developed for my undergraduate thesis, which proposes a heuristic approach for the **Logical Clustering of Data Pipelines**. The problem is formulated as an adaptation of the **Topic-Based Conference Scheduling Problem (TBCSP)** and is solved using metaheuristic optimization techniques.

---

## Technical Specifications

| Component | Version |
|-----------|---------|
| Operating System | Ubuntu 24.04.1 LTS |
| Compiler | G++ 13.3.0 |
| Language | C++ |

---

## Implemented Algorithms

The solution consists of two main stages:

1. **Greedy Algorithm**
   - Generates an initial feasible solution.

2. **Tabu Search**
   - Improves the initial solution using a neighborhood exploration strategy.

---

## Project Structure

> [!NOTE]
> The structure shown is a sample, as the complete structure is too large to display. The latest test results can be found in the `Logs/Algorithm_v10/` and `Logs/Algorithm_v11/` directories.

```text
.
├── Instances/
│   ├── dags_10_s1.csv
│   ├── dags_m0050_s1.csv
│   └── dags_g0250_s1.csv
├── Logs/
│   ├── Algorithm/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_t2/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_t3/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v3/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v4/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v5/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v6/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v7/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v8/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v9/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v10/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v11/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   ├── Algorithm_v12/
│   │   ├── dags_010_s1.txt
│   │   ├── dags_m0050_s1.txt
│   │   └── dags_g0250_s1.txt
│   └── Gurobi/
│       ├── dags_010_s1.txt
│       ├── dags_m0050_s1.tx
│       └── dags_g0250_s1.txt
├── Solved/
│   ├── Algorithm/
│   │   ├── dags_010_s1_cluster_report.csv
│   │   ├── dags_010_s1_critical_tables.csv
│   │   ├── dags_010_s1_solved.csv
│   │   ├── dags_m0050_s1_cluster_report.csv
│   │   ├── dags_m0050_s1_critical_tables.csv
│   │   ├── dags_m0050_s1_solved.csv
│   │   ├── dags_g0250_s1_cluster_report.csv
│   │   ├── dags_g0250_s1_critical_tables.csv
│   │   └── dags_g0250_s1_solved.csv
│   └── Gurobi/
│       ├── dags_010_s1_solved.csv
│       ├── dags_m0050_s1_solved.csv
│       └── dags_g0250_s1_solved.csv 
├── constraints.cpp
├── evaluation.cpp
├── greedy.cpp
├── greedy.h
├── main.cpp
├── reporting.cpp
├── reporting.h
├── run_all_heuristic.sh
├── solver.py
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

To run all instances:

```bash
./run_all_heuristic.sh
```

---

## Input

The input instances are located in the `Instances/` directory. Each CSV file represents a different problem instance.

---

## Output

The generated solutions are stored in the `Solved/Algorithm` directory, including:

* `dags_010_s1_cluster_report.csv`: Summary of the generated clusters.

* `dags_010_s1_critical_tables.csv`: Critical tables identified during optimization.

* `dags_010_s1_solved.csv`: Example of a solved instance.

---

## Experimental Results

The following table compares the results obtained with Gurobi and the proposed metaheuristic across the evaluated instances.

| Instance | Type | Gurobi: Best Solution | Gurobi: gap (%) | Gurobi: Time (s) | Metaheuristic: Best Quality | Metaheuristic: Average Quality | Metaheuristic: Average Time (s) |
|---|---|---:|---:|---:|---:|---:|---:|
| `dags_010_s1` | Corporate | 185 | 0.00 | 0.00 | 185 | 185 | 0.00 |
| `dags_010_s2` | Corporate | 159 | 0.00 | 0.01 | 159 | 159 | 0.00 |
| `dags_015_s1` | Corporate | 378 | 0.00 | 0.07 | 378 | 378 | 0.10 |
| `dags_015_s2` | Corporate | 247 | 0.00 | 0.12 | 247 | 247 | 0.10 |
| `dags_020_s1` | Corporate | 419 | 0.00 | 0.26 | 419 | 419 | 0.10 |
| `dags_020_s2` | Corporate | 363 | 0.00 | 0.28 | 363 | 363 | 0.10 |
| `dags_025_s1` | Corporate | 467 | 0.00 | 1.09 | 467 | 467 | 0.20 |
| `dags_030_s1` | Corporate | 513 | 0.00 | 1483.41 | 513 | 513 | 0.20 |
| `dags_030_s2` | Corporate | 757 | 0.00 | 779.79 | 757 | 757 | 0.20 |
| `dags_040_s1` | Corporate | 881 | 4.31 | 3600.02 | 881 | 881 | 0.40 |
| `dags_050_s1` | Corporate | 1105 | 9.86 | 3600.02 | 1105 | 1105 | 0.60 |
| `dags_050_s2` | Corporate | 1302 | 9.14 | 3600.00 | 1302 | 1302 | 0.50 |
| `dags_075_s1` | Corporate | 1327 | 12.81 | 3600.02 | 1327 | 1327 | 1.30 |
| `dags_100_s1` | Corporate | 1189 | 20.69 | 3611.01 | 1196 | 1196 | 2.20 |
| `dags_100_s2` | Corporate | 1611 | 15.77 | 3600.04 | 1624 | 1624 | 2.20 |
| `dags_150_s1` | Corporate | 1524 | 468.11 | 3600.20 | 1802 | 1802 | 4.90 |
| `dags_208_corp` | Corporate | 1711 | 493.25 | 3600.05 | 2213 | 2213 | 9.30 |
| `dags_208_s2` | Corporate | --- | --- | --- | 2697 | 2697 | 9.40 |
| `dags_300_s1` | Corporate | --- | --- | --- | 2981 | 2981 | 21.30 |
| `dags_500_s1` | Corporate | --- | --- | --- | 4065 | 4065 | 64.20 |
| `dags_p0010_s1` | Synthetic | 13 | 0.00 | 0.01 | 13 | 13 | 0.00 |
| `dags_p0010_s2` | Synthetic | 12 | 0.00 | 0.00 | 12 | 12 | 0.00 |
| `dags_p0010_s3` | Synthetic | 13 | 0.00 | 0.00 | 13 | 13 | 0.00 |
| `dags_p0010_s4` | Synthetic | 13 | 0.00 | 0.01 | 13 | 13 | 0.00 |
| `dags_p0010_s5` | Synthetic | 20 | 0.00 | 0.01 | 20 | 20 | 0.00 |
| `dags_p0015_s1` | Synthetic | 20 | 0.00 | 0.01 | 20 | 20 | 0.10 |
| `dags_p0015_s2` | Synthetic | 18 | 0.00 | 0.07 | 18 | 18 | 0.10 |
| `dags_p0015_s3` | Synthetic | 19 | 0.00 | 0.01 | 19 | 19 | 0.10 |
| `dags_p0015_s4` | Synthetic | 23 | 0.00 | 0.02 | 23 | 23 | 0.10 |
| `dags_p0015_s5` | Synthetic | 22 | 0.00 | 0.01 | 22 | 22 | 0.10 |
| `dags_p0020_s1` | Synthetic | 34 | 0.00 | 0.14 | 34 | 34 | 0.10 |
| `dags_p0020_s2` | Synthetic | 31 | 0.00 | 0.11 | 31 | 31 | 0.10 |
| `dags_p0020_s3` | Synthetic | 25 | 0.00 | 0.06 | 25 | 25 | 0.10 |
| `dags_p0020_s4` | Synthetic | 26 | 0.00 | 0.27 | 26 | 26 | 0.10 |
| `dags_p0020_s5` | Synthetic | 35 | 0.00 | 0.20 | 35 | 35 | 0.10 |
| `dags_p0025_s1` | Synthetic | 34 | 0.00 | 0.27 | 34 | 34 | 0.20 |
| `dags_p0025_s2` | Synthetic | 44 | 0.00 | 0.90 | 44 | 44 | 0.20 |
| `dags_p0025_s3` | Synthetic | 41 | 0.00 | 0.32 | 41 | 41 | 0.20 |
| `dags_p0025_s4` | Synthetic | 43 | 0.00 | 0.65 | 43 | 43 | 0.20 |
| `dags_p0025_s5` | Synthetic | 45 | 0.00 | 0.55 | 45 | 45 | 0.20 |
| `dags_p0030_s1` | Synthetic | 54 | 0.00 | 7.47 | 54 | 54 | 0.20 |
| `dags_p0030_s2` | Synthetic | 59 | 0.00 | 7.86 | 59 | 59 | 0.20 |
| `dags_p0030_s3` | Synthetic | 64 | 0.00 | 6.17 | 64 | 64 | 0.20 |
| `dags_p0030_s4` | Synthetic | 62 | 0.00 | 8.08 | 62 | 62 | 0.20 |
| `dags_p0030_s5` | Synthetic | 58 | 0.00 | 11.14 | 58 | 58 | 0.20 |
| `dags_p0040_s1` | Synthetic | 88 | 0.00 | 760.32 | 88 | 88 | 0.40 |
| `dags_p0040_s2` | Synthetic | 82 | 0.00 | 2757.88 | 82 | 82 | 0.40 |
| `dags_p0040_s3` | Synthetic | 91 | 0.00 | 1987.99 | 91 | 91 | 0.40 |
| `dags_p0040_s4` | Synthetic | 93 | 6.45 | 3600.01 | 93 | 93 | 0.40 |
| `dags_p0040_s5` | Synthetic | 92 | 0.00 | 33.44 | 92 | 92 | 0.40 |
| `dags_m0050_s1` | Synthetic | 115 | 8.70 | 3600.01 | 115 | 115 | 0.60 |
| `dags_m0050_s2` | Synthetic | 124 | 12.90 | 3600.01 | 124 | 124 | 0.60 |
| `dags_m0050_s3` | Synthetic | 122 | 7.38 | 3600.01 | 122 | 122 | 0.60 |
| `dags_m0050_s4` | Synthetic | 116 | 28.45 | 3600.01 | 118 | 118 | 0.70 |
| `dags_m0050_s5` | Synthetic | 127 | 6.30 | 3600.01 | 127 | 127 | 0.60 |
| `dags_m0075_s1` | Synthetic | 215 | 29.77 | 3600.02 | 222 | 222 | 1.30 |
| `dags_m0075_s2` | Synthetic | 184 | 27.72 | 3600.01 | 192 | 192 | 1.30 |
| `dags_m0075_s3` | Synthetic | 195 | 29.74 | 3600.01 | 197 | 197 | 1.30 |
| `dags_m0075_s4` | Synthetic | 180 | 38.89 | 3600.01 | 189 | 189 | 1.30 |
| `dags_m0075_s5` | Synthetic | 194 | 31.96 | 3600.01 | 198 | 198 | 1.30 |
| `dags_m0100_s1` | Synthetic | 293 | 43.00 | 3631.57 | 300 | 300 | 2.30 |
| `dags_m0100_s2` | Synthetic | 271 | 38.38 | 3600.02 | 282 | 282 | 2.30 |
| `dags_m0100_s3` | Synthetic | 290 | 40.00 | 3600.00 | 300 | 300 | 2.20 |
| `dags_m0100_s4` | Synthetic | 276 | 38.77 | 3651.20 | 282 | 282 | 2.30 |
| `dags_m0100_s5` | Synthetic | 290 | 41.38 | 3600.00 | 305 | 305 | 2.20 |
| `dags_m0125_s1` | Synthetic | 367 | 61.31 | 3600.11 | 392 | 392 | 3.40 |
| `dags_m0125_s2` | Synthetic | 372 | 45.43 | 3614.45 | 388 | 388 | 3.50 |
| `dags_m0125_s3` | Synthetic | 366 | 56.01 | 3603.83 | 380 | 380 | 3.60 |
| `dags_m0125_s4` | Synthetic | 348 | 49.14 | 3600.11 | 369 | 369 | 3.50 |
| `dags_m0125_s5` | Synthetic | 374 | 51.34 | 3600.10 | 383 | 383 | 3.50 |
| `dags_m0150_s1` | Synthetic | 415 | 572.29 | 3600.14 | 479 | 479 | 5.00 |
| `dags_m0150_s2` | Synthetic | 410 | 565.37 | 3600.22 | 478 | 478 | 5.00 |
| `dags_m0150_s3` | Synthetic | 417 | 544.60 | 3600.13 | 482 | 482 | 4.90 |
| `dags_m0150_s4` | Synthetic | 444 | 642.79 | 3600.22 | 484 | 484 | 4.90 |
| `dags_m0150_s5` | Synthetic | 390 | 783.59 | 3600.19 | 456 | 456 | 5.00 |
| `dags_m0200_s1` | Synthetic | 593 | 838.11 | 3600.08 | 680 | 680 | 8.90 |
| `dags_m0200_s2` | Synthetic | 598 | 846.32 | 3600.14 | 675 | 675 | 8.80 |
| `dags_m0200_s3` | Synthetic | 629 | 785.85 | 3600.10 | 684 | 684 | 8.60 |
| `dags_m0200_s4` | Synthetic | 583 | 867.24 | 3600.06 | 684 | 684 | 9.10 |
| `dags_m0200_s5` | Synthetic | 541 | 909.61 | 3600.08 | 684 | 684 | 8.80 |
| `dags_g0250_s1` | Synthetic | --- | --- | --- | 855 | 855 | 14.00 |
| `dags_g0250_s2` | Synthetic | --- | --- | --- | 845 | 845 | 13.70 |
| `dags_g0250_s3` | Synthetic | --- | --- | --- | 853 | 853 | 13.70 |
| `dags_g0250_s4` | Synthetic | --- | --- | --- | 847 | 847 | 13.80 |
| `dags_g0250_s5` | Synthetic | --- | --- | --- | 858 | 858 | 13.60 |
| `dags_g0300_s1` | Synthetic | --- | --- | --- | 981 | 981 | 19.80 |
| `dags_g0300_s2` | Synthetic | --- | --- | --- | 963 | 963 | 20.00 |
| `dags_g0300_s3` | Synthetic | --- | --- | --- | 943 | 943 | 20.00 |
| `dags_g0300_s4` | Synthetic | --- | --- | --- | 1002 | 1002 | 20.00 |
| `dags_g0300_s5` | Synthetic | --- | --- | --- | 954 | 954 | 20.00 |
| `dags_g0400_s1` | Synthetic | --- | --- | --- | 1235 | 1235 | 37.10 |
| `dags_g0400_s2` | Synthetic | --- | --- | --- | 1215 | 1215 | 37.20 |
| `dags_g0400_s3` | Synthetic | --- | --- | --- | 1224 | 1224 | 37.20 |
| `dags_g0400_s4` | Synthetic | --- | --- | --- | 1244 | 1244 | 36.70 |
| `dags_g0400_s5` | Synthetic | --- | --- | --- | 1253 | 1253 | 37.30 |
| `dags_g0500_s1` | Synthetic | --- | --- | --- | 1483 | 1483 | 61.60 |
| `dags_g0500_s2` | Synthetic | --- | --- | --- | 1485 | 1485 | 60.80 |
| `dags_g0500_s3` | Synthetic | --- | --- | --- | 1502 | 1502 | 62.20 |
| `dags_g0500_s4` | Synthetic | --- | --- | --- | 1477 | 1477 | 60.70 |
| `dags_g0500_s5` | Synthetic | --- | --- | --- | 1493 | 1493 | 61.70 |
| `dags_g0750_s1` | Synthetic | --- | --- | --- | 2115 | 2115 | 147.80 |
| `dags_g0750_s2` | Synthetic | --- | --- | --- | 2124 | 2124 | 147.10 |
| `dags_g0750_s3` | Synthetic | --- | --- | --- | 2159 | 2159 | 146.30 |
| `dags_g0750_s4` | Synthetic | --- | --- | --- | 2139 | 2139 | 146.80 |
| `dags_g0750_s5` | Synthetic | --- | --- | --- | 2123 | 2123 | 147.60 |
| `dags_g1000_s1` | Synthetic | --- | --- | --- | 2473 | 2473 | 274.40 |
| `dags_g1000_s2` | Synthetic | --- | --- | --- | 2724 | 2724 | 275.90 |
| `dags_g1000_s3` | Synthetic | --- | --- | --- | 2629 | 2629 | 283.20 |
| `dags_g1000_s4` | Synthetic | --- | --- | --- | 2718 | 2718 | 280.60 |
| `dags_g1000_s5` | Synthetic | --- | --- | --- | 2722 | 2722 | 276.80 |

> [!NOTE]  
> `---` indicates that Gurobi did not report a solution for the instance.

---

## Thesis

This implementation accompanies the undergraduate thesis:

**"Acercamiento heurístico para el agrupamiento lógico de pipelines de datos"**

The proposed methodology adapts the **Topic-Based Conference Scheduling Problem (TBCSP)** to address the logical clustering of data pipelines through heuristic optimization.