include ./Makefile.inc
#### WHERE THE TEST BINARY IS LOCATED
TEST=./test
#### WHERE THE SOURCE BINARIES ARE LOCATED
BIN=./bin

#### WHERE THE SOURCE FILES ARE LOCATED
SRC=./src
mainC=main.c
viewC=view.c
baseBIN=$(mainC:.c=.o)
viewBIN=$(viewC:.c=.o)

all:
	@cd $(SRC); make all; cd ..
	@$(GCC) $(GCCFLAGS) -o $(baseBIN) $(mainC)
	@$(GCC) $(GCCFLAGS) -o $(viewBIN) $(viewC)

try: all
	@echo "Running test..."
	@echo "===================="
	@echo " "
	@cd $(TEST); ../$(baseBIN) * | ../$(viewBIN)
	@./$(baseBIN) | ./$(viewBIN)
	@echo " "
	@echo "===================="
	@echo "Done."
	@make clean

clean:
	@echo "Cleaning up..."
	@rm $(baseBIN)
	@rm $(viewBIN)
	@cd $(SRC); make clean
	@echo "Done."