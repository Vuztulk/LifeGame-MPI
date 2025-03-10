# LifeGame-MPI

Una implementación paralela del **Juego de la Vida** (Conway's Game of Life) utilizando MPI (Message Passing Interface). Este proyecto demuestra cómo distribuir el cálculo de cada generación del autómata celular entre múltiples procesos, aprovechando la paralelización para mejorar el rendimiento en simulaciones de grandes dimensiones.

## Descripción

El Juego de la Vida es un autómata celular ideado por John Conway. Se trata de un sistema discreto que evoluciona en función de un conjunto de reglas simples aplicadas a una cuadrícula de celdas. En este proyecto, se utiliza MPI para dividir la cuadrícula entre varios procesos, permitiendo la simulación de manera concurrente y eficiente.

## Características

- **Paralelización:** Distribución del cómputo mediante MPI para aprovechar múltiples núcleos o nodos.
- **Escalabilidad:** Capacidad para simular cuadrículas de gran tamaño y muchas generaciones.
- **Modularidad:** Código estructurado para facilitar modificaciones y la inclusión de nuevas funcionalidades.

## Requisitos

- **MPI:** Se recomienda utilizar [OpenMPI](https://www.open-mpi.org/) o [MPICH](https://www.mpich.org/).
- **Compilador C/C++:** Se requiere un compilador compatible (por ejemplo, `gcc` o `clang`).
