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

all: slave
	@$(GCC) $(GCCFLAGS) -o $(baseBIN) $(mainC)
	# @$(GCC) $(GCCFLAGS) -o $(viewBIN) $(viewC)

slave: 
	@cd $(SRC); make all; cd ..
try: all
	@echo "Running test..."
	@echo "===================="
	@echo " "
	@cd $(TEST); ../$(baseBIN) *
	@echo " "
	@echo "===================="
	@echo "Done."
	@make clean

try_slave: slave
	@echo "Running test..."
	@echo "===================="
	@echo " "
	@cd $(TEST); ../src/bin/slave.o test.txt
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