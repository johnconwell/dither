BDIR = ./bin
IDIR = ./include
ODIR = ./obj
SDIR = ./src
TDIR = ./tests

CC = g++
# -Wsign-conversion catches signed/unsigned mix-ups such as REVIEW B1
WARNINGS = -Wall -Wextra -Wconversion -Wsign-conversion
CFLAGS = -I$(IDIR) -std=c++23 -O2 $(WARNINGS) -MMD -MP

# on Windows (MinGW) the linker appends .exe, so the target name must include it
ifeq ($(OS),Windows_NT)
    EXE = .exe
else
    EXE =
endif

TARGET = $(BDIR)/dither$(EXE)
TEST_TARGET = $(BDIR)/test_dither$(EXE)

# list of objects and rule to format them as [OBJECT_DIR]/[OBJECT]
_OBJ = benchmark.o cli.o color.o convolve.o csv.o dither.o error_diffusion.o gif.o grayscale.o image.o lodepng.o main.o ordered.o ordered_tables.o palette.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# every object except main.o, linked into the test runner
LIB_OBJ = $(filter-out $(ODIR)/main.o,$(OBJ))
TEST_OBJ = $(ODIR)/test_main.o

.PHONY : all dither test clean

all : $(TARGET)

# kept so that `make dither` still works
dither : $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# vendored third-party code (lodepng, gif-h): don't report its warnings
$(ODIR)/lodepng.o $(ODIR)/gif.o : WARNINGS = -w

# rule to make each object from the matching cpp file; header dependencies come from the generated .d files
$(ODIR)/%.o : $(SDIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/test_main.o : $(TDIR)/test_main.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(TEST_TARGET) : $(LIB_OBJ) $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# the CLI tests run the real binary, so build both
test : $(TARGET) $(TEST_TARGET)
	$(TEST_TARGET) $(TARGET)

clean :
	rm -f $(BDIR)/dither $(BDIR)/test_dither $(BDIR)/*.exe $(ODIR)/*.o $(ODIR)/*.d
	rm -rf $(TDIR)/tmp

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
