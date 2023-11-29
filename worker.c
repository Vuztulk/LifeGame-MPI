#include "worker.h"

void workerLogic(int rank, int worldWidth, int worldHeight, int totalIterations, int autoMode, int modeStatic, int grainSize, int n_proc) {
    
    int start, rows, final = 0, worker_finished = 1;
    MPI_Status status;
    unsigned short* myWorld;
    unsigned short* newWorld;
    unsigned short* top;
    unsigned short* bottom;

    while (!final) {

        MPI_Recv(&start, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&rows, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

        top = (unsigned short*)malloc(worldWidth * sizeof(unsigned short));
        myWorld = (unsigned short*)malloc(rows * worldWidth * sizeof(unsigned short));
        bottom = (unsigned short*)malloc(worldWidth * sizeof(unsigned short));
        newWorld = (unsigned short*)malloc(rows * worldWidth * sizeof(unsigned short));

        MPI_Recv(top, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(myWorld, rows * worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(bottom, worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD, &status);

        calcular(top,myWorld,bottom, newWorld, rows, worldWidth); 

        if(!modeStatic)
            MPI_Send(&worker_finished, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);

        MPI_Send(&start, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
        MPI_Send(&rows, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
        MPI_Send(newWorld, rows * worldWidth, MPI_UNSIGNED_SHORT, MASTER, 0, MPI_COMM_WORLD);
        
        MPI_Recv(&final, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &status);

        free(top);
        free(myWorld);
        free(bottom);
        free(newWorld);
    }
    
	MPI_Finalize();
}


void calcular(unsigned short* top, unsigned short* myWorld, unsigned short* bottom, unsigned short* newWorld, int rows, int worldWidth) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < worldWidth; col++) {

            tCoordinate c = {row, col};
            int liveNeighbors = 0;

            tCoordinate* neighbors[8] = {getCellUp(&c), getCellDown(&c), getCellLeft(&c, worldWidth), getCellRight(&c, worldWidth),
            getCellUp(getCellLeft(&c, worldWidth)), getCellUp(getCellRight(&c, worldWidth)), getCellDown(getCellLeft(&c, worldWidth)),
            getCellDown(getCellRight(&c, worldWidth))};
            
            for (int i = 0; i < 8; i++) {
                unsigned short cell;
                // Comprobamos si estamos en los bordes del mundo
                if ((i < 2 && row == 0) || (i > 1 && i < 4 && col == 0) || (i > 3 && i < 6 && row == 0) || (i > 5 && row == rows - 1)) {
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
                // Sumamos 1 a liveNeighbors si la celda está viva
                liveNeighbors += (cell == CELL_LIVE || cell == CELL_NEW) ? 1 : 0;
                // Liberamos la memoria de la celda vecina
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



