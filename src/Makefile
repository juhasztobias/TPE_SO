include ../Makefile.inc
# To build src files and export them into bin directory
SRC_FILES=$(wildcard *.c)
OBJ_FILES=$(SRC_FILES:.c=.o)

all: $(OBJ_FILES)
	@echo "Compiled all source files"
	@echo "Use 'make clean' to remove object files"
	@echo "Use 'make run' to run the program"

%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p bin
	@$(GCC) $(GCCFLAGS) $< -o ./bin/$@
clean:
	@rm -R bin
	@echo "Cleaned up object files"