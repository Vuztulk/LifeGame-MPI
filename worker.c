#include "worker.h"

void workerLogic(int rank, int worldWidth, int worldHeight, int totalIterations, int autoMode, int modeStatic, int grainSize, int n_proc) {
    
    int desplazamiento, rowsPerProcess, final = 0;
    MPI_Status status;

    unsigned short* top = (unsigned short*)malloc(worldWidth * sizeof(unsigned short));
    unsigned short* bottom = (unsigned short*)malloc(worldWidth * sizeof(unsigned short));
    unsigned short* myWorld = (unsigned short*)malloc(worldHeight * worldWidth * sizeof(unsigned short));
    unsigned short* newWorld = (unsigned short*)malloc(worldHeight * worldWidth * sizeof(unsigned short));

    while (!final) {

        MPI_Recv(&desplazamiento, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&rowsPerProcess, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

        MPI_Recv(top, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(myWorld, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(bottom, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);

        calcular(top, myWorld, bottom, newWorld, rowsPerProcess, worldWidth); 

        MPI_Send(&desplazamiento, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
        MPI_Send(&rowsPerProcess, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
        MPI_Send(newWorld, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD);
        
        MPI_Recv(&final, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);
    }

    free(top);
    free(myWorld);
    free(bottom);
    free(newWorld);
}

void calcular(unsigned short* top, unsigned short* myWorld, unsigned short* bottom, unsigned short* newWorld, int desplazamiento, int worldWidth) {
    for (int row = 0; row < desplazamiento; row++) {
        for (int col = 0; col < worldWidth; col++) {

            tCoordinate c = {row, col};
            int liveNeighbors = 0;

            tCoordinate* neighbors[8] = {getCellUp(&c), getCellDown(&c), getCellLeft(&c, worldWidth), getCellRight(&c, worldWidth),
            getCellUp(getCellLeft(&c, worldWidth)), getCellUp(getCellRight(&c, worldWidth)), getCellDown(getCellLeft(&c, worldWidth)),
            getCellDown(getCellRight(&c, worldWidth))};
            
            for (int i = 0; i < 8; i++) {
                unsigned short cell;
                // Comprobamos si estamos en los bordes del mundo
                if ((i < 2 && row == 0) || (i > 1 && i < 4 && col == 0) || (i > 3 && i < 6 && row == 0) || (i > 5 && row == desplazamiento - 1)) {
                    // Si estamos en los bordes, usamos las celdas de los arrays top o bottom
                    if (i % 4 == 0 || i == 5) {
                        cell = top[neighbors[i]->col];
                    } else {
                        cell = bottom[neighbors[i]->col];
                    }
                } else {
                    // Si no estamos en los bordes, obtenemos la celda del mundo
                    cell = getCellAtWorld(neighbors[i], myWorld, worldWidth);
                }
                
                liveNeighbors += (cell == CELL_LIVE || cell == CELL_NEW) ? 1 : 0;
                free(neighbors[i]);
            }

            if(liveNeighbors < 1){
                calculateLonelyCell();
            }

            unsigned short currentCell = getCellAtWorld(&c, myWorld, worldWidth);
            unsigned short newCell;
            switch (currentCell) {
                case CELL_LIVE:
                case CELL_NEW:
                    newCell = (liveNeighbors == 2 || liveNeighbors == 3) ? CELL_LIVE : CELL_DEAD;
                    break;

                case CELL_DEAD:
                case CELL_EMPTY:
                    newCell = (liveNeighbors == 3) ? CELL_NEW : CELL_EMPTY;
                    break;

                case CELL_CATACLYSM:
                    newCell = CELL_EMPTY;
                    break;
            }

            setCellAt(&c, newWorld, worldWidth, newCell);
        }
    }
}



