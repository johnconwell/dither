BDIR = ./bin
IDIR = ./include
LDIR = ./lib
ODIR = ./obj
SDIR = ./src
OUTDIR = ./output
# CTNDIR = $(OUTDIR)/convolution
# CVDDIR = $(OUTDIR)/convolve_dither
# EDDIR = $(OUTDIR)/error_diffusion
# ORDDIR = $(OUTDIR)/ordered
# TMDIR = $(OUTDIR)/threshold_matrix

CC = g++
CFLAGS = -I$(IDIR) -L$(LDIR) -Wall -std=c++26

LIBS = #-llibfftw3-3 -llibfftw3f-3 -llibfftw3l-3 -lm

# list of headers (dependencies) and rule to format them as [INCLUDE_DIR]/[HEADER]
_DEPS = benchmark.h cli.h color.h dither.h gif.h image.h lodepng.h palette.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# list of objects and rule to format them as [OBJECT_DIR]/[OBJECT]
_OBJ = benchmark.o cli.o color.o dither.o gif.o image.o lodepng.o main.o palette.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# rule to make each object with corresponding named cpp file and headers as dependencies
$(ODIR)/%.o : $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# rule to make all objects and build the result as main.exe
dither : $(OBJ)
	$(CC) -o $(BDIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY : clean

clean:
	rm -f $(BDIR)/*.exe $(ODIR)/*.o $(OUTDIR)/*.png $(OUTDIR)/*.gif
