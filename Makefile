BDIR = ./bin
IDIR = ./include
LDIR = ./lib
ODIR = ./obj
SDIR = ./src

CC = g++
CFLAGS = -I$(IDIR) -L$(LDIR) -Wall -std=c++26

LIBS = -llibfftw3-3 -llibfftw3f-3 -llibfftw3l-3 -lm

# list of headers (dependencies) and rule to format them as [INCLUDE_DIR]/[HEADER]
_DEPS = bayer.h blue_noise.h brown_noise.h color.h dither.h  fftw3.h fourier.h grayscale.h image.h lodepng.h palette.h white_noise.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# list of objects and rule to format them as [OBJECT_DIR]/[OBJECT]
_OBJ = bayer.o blue_noise.o brown_noise.o color.o dither.o error_diffusion.o fourier.o grayscale.o image.o lodepng.o main.o palette.o white_noise.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# rule to make each object with corresponding named cpp file and headers as dependencies
$(ODIR)/%.o : $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# rule to make all objects and build the result as main.exe
main : $(OBJ)
	$(CC) -o $(BDIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY : clean

clean:
	rm -f $(ODIR)/*.o
