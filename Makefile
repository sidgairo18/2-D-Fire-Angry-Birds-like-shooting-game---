CC = g++
CFLAGS = -Wall
PROG = exe

SRCS = game.cpp
LIBS = glad.c -I /usr/local/include/GLFW -lGL  -lglfw -lX11 -lXxf86vm -lXrandr  -lXi -ldl 

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
