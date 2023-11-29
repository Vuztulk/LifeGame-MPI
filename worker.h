#include "mpi.h"
#include "world.h"

// Enables/Disables the log messages from worker processes
#define DEBUG_WORKER 0

void workerLogic(int rank, int worldWidth, int worldHeight, int totalIterations, int autoMode, int modeStatic, int grainSize, int n_proc);

void calcular(unsigned short* top, unsigned short* myWorld, unsigned short* bottom, unsigned short* newWorld, int rows, int worldWidth);
