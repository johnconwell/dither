CC = g++

OBJS = color.cpp dither.cpp grayscale.cpp image.cpp lodepng.cpp main.cpp

OBJ_NAME = main

all : $(OBJS)
	$(CC) $(OBJS) -o $(OBJ_NAME)

clean :
	rm $(OBJ_NAME)