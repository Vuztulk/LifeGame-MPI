CC = mpicc
CFLAGS = -I/usr/local/sdl2/include/SDL2 -g
LDFLAGS = -L/usr/local/sdl2/lib/
LIBS = -lSDL2
TARGET = lifeGame

all:
	$(CC) -o $(TARGET) *.c $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
