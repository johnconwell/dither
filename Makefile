BDIR = ./bin
IDIR = ./include
LDIR = ./lib
ODIR = ./obj
SDIR = ./src

CC = g++
CFLAGS = -I$(IDIR) -Wall -std=c++26

LIBS = -lm

_DEPS = bayer.h blue_noise.h brown_noise.h color.h dither.h error.h fftw3.h fourier.h grayscale.h image.h lodepng.h ordered.h palette.h white_noise.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = bayer.o blue_noise.o brown_noise.o color.o dither.o error.o fourier.o grayscale.o image.o lodepng.o main.o ordered.o palette.o white_noise.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o : $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main : $(OBJ)
	$(CC) -o $(BDIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY : clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
