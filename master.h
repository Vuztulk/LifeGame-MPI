#include "graph.h"
#include "mpi.h"

// Enables/Disables the log messages from the master process
#define DEBUG_MASTER 0

// Probability that a cataclysm may occur [0-100] :(
#define PROB_CATACLYSM 50

// Number of iterations between two possible cataclysms
#define ITER_CATACLYSM 5

void masterLogic(SDL_Window* window, SDL_Renderer* renderer, int worldWidth, int worldHeight, int totalIterations, int autoMode, char* outputFile, int modeStatic, int grainn_proc, int n_proc);

void printWorld(unsigned short** current, unsigned short** next, SDL_Renderer* renderer, int worldHeight, int worldWidth, int autoMode, int iteration, SDL_Window* window);

void initWorld(SDL_Window* window, SDL_Renderer* renderer, int worldWidth, int worldHeight, unsigned short** currentWorld, unsigned short** newWorld, int autoMode);

void cargaEstatica(int n_proc, int remaining, int currentRow, int worldHeight, unsigned short* newWorld, int worldWidth);

void recibeEstatica(int n_proc, unsigned short* newWorld, int worldWidth, int final);

void cargaDinamica(int n_proc, int grainSize, unsigned short* currentWorld, unsigned short* newWorld, int worldWidth, int worldHeight, int final);

int recibeDinamica(unsigned short* newWorld, int worldWidth, int final, int * processed);

void sendDinamica(int i, int rowsPerProcess, int desplazamiento, unsigned short* currentWorld, int worldWidth, int worldHeight);

void checkCataclysm(unsigned short* myWorld, int worldWidth, int worldHeight, int iteration);

void printMatriz(unsigned short* world, int worldWidth, int worldHeight);

int allProcessed(int* processed, int n_proc);

