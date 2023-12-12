#include "master.h"

void masterLogic(SDL_Window* window, SDL_Renderer* renderer, int worldWidth, int worldHeight, int totalIterations, int autoMode, char* outputFile, int modeStatic, int grainSize, int n_proc) {

    int final = 0;

    unsigned short* currentWorld = (unsigned short*)malloc(worldWidth * worldHeight * sizeof(unsigned short));
    unsigned short* newWorld = (unsigned short*)malloc(worldWidth * worldHeight * sizeof(unsigned short));

    initWorld(window, renderer, worldWidth, worldHeight, &currentWorld, &newWorld, autoMode);

    for (int iteration = 0; iteration < totalIterations; iteration++) {
        
        final = iteration >= totalIterations - 1 ? 1 : 0;

        checkCataclysm(currentWorld, worldWidth, worldHeight,iteration);
        
        if (!modeStatic) {
            cargaDinamica(n_proc, grainSize, currentWorld, newWorld, worldWidth, worldHeight, final);
        } 
        else {
            cargaEstatica(n_proc, worldHeight,worldWidth, currentWorld, newWorld, final);
        }
        printWorld(&currentWorld, &newWorld, renderer, worldHeight, worldWidth, autoMode, iteration, window);
    }

    //saveImage(renderer, outputFile, worldWidth * 400, worldHeight * 400); //Guarda imagen

    free(currentWorld);
    free(newWorld);

}

void cargaEstatica(int n_proc, int worldHeight, int worldWidth,unsigned short* currentWorld, unsigned short* newWorld, int final) {
    
    int extra = worldHeight % (n_proc - 1); // Calcula el excedente
    int desplazamiento, rowsPerProcess, currentRow = 0;

    unsigned short* top;
    unsigned short* area;
    unsigned short* bottom;

    for (int i = 1; i < n_proc; i++) {
        rowsPerProcess = worldHeight / (n_proc - 1);
        if (extra > 0) { // Si hay excedente, añade una fila extra a este proceso
            rowsPerProcess++;
            extra--;
        }
        desplazamiento = currentRow * worldWidth;
        currentRow += rowsPerProcess;

        int topIndex = (desplazamiento - worldWidth < 0) ? worldHeight * worldWidth - worldWidth : desplazamiento - worldWidth;
        int bottomIndex = (desplazamiento + rowsPerProcess * worldWidth >= worldHeight * worldWidth) ? 0 : desplazamiento + rowsPerProcess * worldWidth;

        top = currentWorld + topIndex;
        area = currentWorld + desplazamiento;
        bottom = currentWorld + bottomIndex;

        MPI_Send(&desplazamiento, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&rowsPerProcess, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        MPI_Send(area, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    }
    recibeEstatica(n_proc, newWorld, worldWidth, final);

}

void recibeEstatica(int n_proc, unsigned short* newWorld, int worldWidth, int final) {

    int desplazamiento, rowsPerProcess;
    MPI_Status status;

    for (int i = 1; i < n_proc; i++) {

        MPI_Recv(&desplazamiento, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&rowsPerProcess, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Recv(newWorld + desplazamiento, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&final, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        
    }

}

void cargaDinamica(int n_proc, int grainSize, unsigned short* currentWorld, unsigned short* newWorld, int worldWidth, int worldHeight, int final) {
    
    int enviar = worldHeight, recibir = worldHeight, currentRow = 0, acabar, desplazamiento, rowsPerProcess;
    MPI_Status status;
    
    // Distribucion inicial
    for (int i = 1; i < n_proc && enviar > 0; i++) {
        rowsPerProcess = grainSize < enviar ? grainSize : enviar;
        desplazamiento = currentRow * worldWidth;
        enviar -= rowsPerProcess;
        currentRow += rowsPerProcess;
        
        sendDinamica(i, rowsPerProcess, desplazamiento, currentWorld, worldWidth, worldHeight);
    }
    
    while (recibir > 0) {

        acabar = (final == 1) && (enviar == 0) ? 1 : 0;

        int worker = recibeDinamica(newWorld, worldWidth, acabar, &recibir);

        if (enviar > 0) {
            rowsPerProcess = grainSize < enviar ? grainSize : enviar;
            desplazamiento = currentRow * worldWidth;
            enviar -= rowsPerProcess;
            currentRow += rowsPerProcess;

            sendDinamica(worker, rowsPerProcess, desplazamiento, currentWorld, worldWidth, worldHeight);
        }
    }

}

void sendDinamica(int i, int rowsPerProcess, int desplazamiento, unsigned short* currentWorld, int worldWidth, int worldHeight) {

    int topIndex = (desplazamiento - worldWidth < 0) ? worldHeight * worldWidth - worldWidth : desplazamiento - worldWidth;
    int bottomIndex = (desplazamiento + rowsPerProcess * worldWidth >= worldHeight * worldWidth) ? 0 : desplazamiento + rowsPerProcess * worldWidth;

    unsigned short* top = currentWorld + topIndex;
    unsigned short* area = currentWorld + desplazamiento;
    unsigned short* bottom = currentWorld + bottomIndex;

    MPI_Send(&desplazamiento, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    MPI_Send(&rowsPerProcess, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    MPI_Send(area, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);

}

int recibeDinamica(unsigned short* newWorld, int worldWidth, int final, int* recibir) {

    int desplazamiento, rowsPerProcess;
    MPI_Status status;

    MPI_Recv(&desplazamiento, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&rowsPerProcess, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    *recibir -= rowsPerProcess;

    MPI_Recv(newWorld + desplazamiento, rowsPerProcess * worldWidth, MPI_UNSIGNED_SHORT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Send(&final, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

    return status.MPI_SOURCE;

}

void printWorld(unsigned short** current, unsigned short** next, SDL_Renderer* renderer, int worldHeight, int worldWidth, int autoMode, int iteration, SDL_Window* window) {
    
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);

    pintaWorld(*current, renderer, 0, worldHeight - 1, worldWidth);

    SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
    
    unsigned short* temp = *current;
    *current = *next;
    *next = temp;
    
    if (!autoMode == 1) { // Step mode
        printf("Presione cualquier tecla para continuar...\n");
        char ch = getchar();
    }

}

void initWorld(SDL_Window* window, SDL_Renderer* renderer, int worldWidth, int worldHeight, unsigned short** currentWorld, unsigned short** newWorld, int autoMode) {
    
    clearWorld(*currentWorld, worldWidth, worldHeight);
    clearWorld(*newWorld, worldWidth, worldHeight);
    initRandomWorld(*currentWorld, worldWidth, worldHeight);

    printf("\nMundo inicializado\n");

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    pintaWorld(*currentWorld, renderer, 0, worldHeight - 1, worldWidth);
    SDL_RenderPresent(renderer);
    SDL_UpdateWindowSurface(window);

    printf("Mundo creado (iteration 0)\n");
    if(autoMode){
        printf("Presione cualquier tecla para iniciar...\n");
        char ch = getchar();
    }
}

void checkCataclysm(unsigned short* myWorld, int worldWidth, int worldHeight, int iteration) {

    if (iteration % ITER_CATACLYSM == 1 && (rand() % 101 < PROB_CATACLYSM)) {
        for (int row = 0; row < worldHeight; row++) {
            myWorld[row * worldWidth] = myWorld[row * worldWidth + worldWidth - 1] = CELL_CATACLYSM;
        }
    }
    
}