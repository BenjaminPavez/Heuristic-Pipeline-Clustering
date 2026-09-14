import csv
import os


for arch in os.listdir('Instances/'):
    if arch.endswith('.csv') and arch != 'important_tables.csv':
        num_dags = 0

        tablas_source = set()
        tablas_target = set()

        with open(os.path.join('Instances', arch), 'r', newline='', encoding='utf-8') as csvfile:

            reader = csv.reader(csvfile, delimiter=';')
            next(reader)

            for dag_name, NumTablasAlimenta, TablasAlimenta, NumTablasAlimentan, TablasAlimentan in reader:

                num_dags += 1

                if TablasAlimenta:
                    for tabla in TablasAlimenta.split(','):
                        tabla = tabla.strip()

                        if tabla:
                            tablas_target.add(tabla)

                # Tablas target
                if TablasAlimentan:
                    for tabla in TablasAlimentan.split(','):
                        tabla = tabla.strip()

                        if tabla:
                            tablas_source.add(tabla)

        nombre_instancia = os.path.splitext(arch)[0]

        if nombre_instancia.startswith('dags_'):
            tipo_instancia = 'Sintética'
        else:
            tipo_instancia = 'Corporativa'

        tablas_totales = tablas_source | tablas_target

        print(f"{nombre_instancia};{tipo_instancia};{num_dags};{len(tablas_target)};{len(tablas_source)};{len(tablas_totales)}")