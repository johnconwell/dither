CC = g++

OBJS = bayer.cpp blue_noise.cpp brown_noise.cpp color.cpp dither.cpp error.cpp grayscale.cpp image.cpp lodepng.cpp main.cpp ordered.cpp palette.cpp white_noise.cpp

OBJ_NAME = main

COMPILER_FLAGS = -std=c++26

all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) -o $(OBJ_NAME)

clean :
	rm $(OBJ_NAME)