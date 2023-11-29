#include "master.h"

void masterLogic(SDL_Window* window, SDL_Renderer* renderer, int worldWidth, int worldHeight, int totalIterations, int autoMode, char* outputFile, int modeStatic, int grainSize, int n_proc) {
    
    int remaining = worldHeight; // Filas restantes a procesar
    int currentRow = 0, final = 0; // Fila por la que vamos procesando

    unsigned short* currentWorld = (unsigned short*)malloc(worldWidth * worldHeight * sizeof(unsigned short));
    unsigned short* newWorld = (unsigned short*)malloc(worldWidth * worldHeight * sizeof(unsigned short));

    initWorld(window, renderer, worldWidth, worldHeight, &currentWorld, &newWorld, autoMode);

    int* rowsPerProcess = (int*)malloc(n_proc * sizeof(int));
    int* desplazamiento = (int*)malloc(n_proc * sizeof(int));
    int rowsPerWorker = modeStatic ? worldHeight / (n_proc - 1) : grainSize;

    for (int iteration = 0; iteration < totalIterations; iteration++) {
        
        final = iteration >= totalIterations - 1 ? 1 : 0;

        checkCataclysm(currentWorld, worldWidth, worldHeight,iteration);
        
        if (!modeStatic) {
            cargaDinamica(n_proc, grainSize, currentWorld,newWorld, worldWidth, worldHeight, final);
        } 
        else {
            cargaEstatica(n_proc, remaining, currentRow, worldHeight, currentWorld, rowsPerProcess, desplazamiento, worldWidth);
            recibeEstatica(n_proc, desplazamiento, rowsPerProcess, newWorld, worldWidth, final);
        }
        printWorld(&currentWorld, &newWorld, renderer, worldHeight, worldWidth, autoMode, iteration, window);
    }

    //saveImage(renderer, outputFile, worldWidth * 400, worldHeight * 400); //Guarda imagen

    free(currentWorld);
    free(newWorld);
    free(rowsPerProcess);
    free(desplazamiento);
}

void cargaEstatica(int n_proc, int remaining, int currentRow, int worldHeight, unsigned short* currentWorld, int* rowsPerProcess, int* desplazamiento, int worldWidth) {
    
    int extra = worldHeight % (n_proc - 1); // Calcula el excedente

    for (int i = 1; i < n_proc; i++) {
        rowsPerProcess[i] = worldHeight / (n_proc - 1);
        if (extra > 0) { // Si hay excedente, añade una fila extra a este proceso
            rowsPerProcess[i]++;
            extra--;
        }
        desplazamiento[i] = currentRow * worldWidth;
        remaining -= rowsPerProcess[i];
        currentRow += rowsPerProcess[i];

        int topIndex = (desplazamiento[i] - worldWidth < 0) ? worldHeight * worldWidth - worldWidth : desplazamiento[i] - worldWidth;
        int bottomIndex = (desplazamiento[i] + rowsPerProcess[i] * worldWidth >= worldHeight * worldWidth) ? 0 : desplazamiento[i] + rowsPerProcess[i] * worldWidth;

        unsigned short* top = currentWorld + topIndex;
        unsigned short* area = currentWorld + desplazamiento[i];
        unsigned short* bottom = currentWorld + bottomIndex;

        MPI_Send(&desplazamiento[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&rowsPerProcess[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        MPI_Send(area, rowsPerProcess[i] * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
        MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    }
}


void recibeEstatica(int n_proc, int* desplazamiento, int* rowsPerProcess, unsigned short* newWorld, int worldWidth, int final) {
    
    for (int i = 1; i < n_proc; i++) {
        MPI_Recv(&desplazamiento[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&rowsPerProcess[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        unsigned short* tempWorld = (unsigned short*) malloc(rowsPerProcess[i] * worldWidth * sizeof(unsigned short));
        MPI_Recv(tempWorld, rowsPerProcess[i] * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Send(&final, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        memcpy(newWorld + desplazamiento[i], tempWorld, rowsPerProcess[i] * worldWidth * sizeof(unsigned short));
        free(tempWorld);
    }
}

void cargaDinamica(int n_proc, int grainSize, unsigned short* currentWorld, unsigned short* newWorld, int worldWidth, int worldHeight, int final) {
    
    int worker_finished, remaining = worldHeight, currentRow = 0, last_worker = -1, acabar = 0;
    MPI_Status status;

    int* rowsPerProcess = (int*)malloc(n_proc * sizeof(int));
    int* desplazamiento = (int*)malloc(n_proc * sizeof(int));
    int* processed = (int*)malloc(n_proc * sizeof(int));
    memset(processed, 1, n_proc * sizeof(int));
    
    // Distribucion inicial
    for (int i = 1; i < n_proc && remaining > 0; i++) {
        rowsPerProcess[i] = grainSize < remaining ? grainSize : remaining;
        desplazamiento[i] = currentRow * worldWidth;
        remaining -= rowsPerProcess[i];
        currentRow += rowsPerProcess[i];
        
        sendDinamica(i, rowsPerProcess, desplazamiento, currentWorld, worldWidth, worldHeight);
        processed[i] = 0; // No ha sido procesado aun
    }
    
    while (remaining > 0 || !allProcessed(processed, n_proc)) {
        MPI_Recv(&worker_finished, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        last_worker = status.MPI_SOURCE;
        processed[last_worker] = 1;

        acabar = (final == 1) && (remaining == 0) ? 1 : 0;

        recibeDinamica(last_worker, desplazamiento, rowsPerProcess, newWorld, worldWidth, acabar);

        if (remaining > 0) {
            rowsPerProcess[last_worker] = grainSize < remaining ? grainSize : remaining;
            desplazamiento[last_worker] = currentRow * worldWidth;
            remaining -= rowsPerProcess[last_worker];
            currentRow += rowsPerProcess[last_worker];

            sendDinamica(last_worker, rowsPerProcess, desplazamiento, currentWorld, worldWidth, worldHeight);
            processed[last_worker] = 0;
        }
    }
    
    free(rowsPerProcess);
    free(desplazamiento);
    free(processed);
}

int allProcessed(int* processed, int n_proc) {
    int allDone = 1;

    for (int i = 1; i < n_proc && allDone; i++) {
        if (processed[i] == 0) {
            allDone = 0;
        }
    }

    return allDone;
}

void sendDinamica(int i, int* rowsPerProcess, int* desplazamiento, unsigned short* currentWorld, int worldWidth, int worldHeight) {

    int topIndex = (desplazamiento[i] - worldWidth < 0) ? worldHeight * worldWidth - worldWidth : desplazamiento[i] - worldWidth;
    int bottomIndex = (desplazamiento[i] + rowsPerProcess[i] * worldWidth >= worldHeight * worldWidth) ? 0 : desplazamiento[i] + rowsPerProcess[i] * worldWidth;

    unsigned short* top = currentWorld + topIndex;
    unsigned short* area = currentWorld + desplazamiento[i];
    unsigned short* bottom = currentWorld + bottomIndex;

    MPI_Send(&desplazamiento[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    MPI_Send(&rowsPerProcess[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    MPI_Send(top, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    MPI_Send(area, rowsPerProcess[i] * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
    MPI_Send(bottom, worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD);
}

void recibeDinamica(int i, int* desplazamiento, int* rowsPerProcess, unsigned short* newWorld, int worldWidth, int final) {
    
    MPI_Recv(&desplazamiento[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&rowsPerProcess[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    unsigned short* tempWorld = (unsigned short*) malloc(rowsPerProcess[i] * worldWidth * sizeof(unsigned short));
    MPI_Recv(tempWorld, rowsPerProcess[i] * worldWidth, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Send(&final, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

    memcpy(newWorld + desplazamiento[i], tempWorld, rowsPerProcess[i] * worldWidth * sizeof(unsigned short));
    free(tempWorld);
    
}

void printWorld(unsigned short** current, unsigned short** next, SDL_Renderer* renderer, int worldHeight, int worldWidth, int autoMode, int iteration, SDL_Window* window) {
    
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
	SDL_RenderClear(renderer);
    //printf("Iteracion: %d\n",iteration);
    //printMatriz(*current,worldWidth,worldHeight);
    //printMatriz(*next,worldWidth,worldHeight);

    pintaWorld(*current, renderer, 0, worldHeight - 1, worldWidth);

    SDL_RenderPresent(renderer);
	SDL_UpdateWindowSurface(window);
    
    unsigned short* temp = *current;
    *current = *next;
    *next = temp;
    
    if (autoMode == 1) { // Auto mode
        //SDL_Delay(80);
    } else { // Step mode
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
            myWorld[row * worldWidth] = CELL_CATACLYSM;
            myWorld[row * worldWidth + worldWidth - 1] = CELL_CATACLYSM;
        }
    }
}

void printMatriz(unsigned short* world, int worldWidth, int worldHeight){
    for (int row = 0; row < worldHeight; row++) {
        for (int col = 0; col < worldWidth; col++) {
            printf("%hu ", world[row * worldHeight + col]);
        }  
        printf("\n");
    }
    printf("\n");
}