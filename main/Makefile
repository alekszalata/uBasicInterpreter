CFLAGS = -std=c11 -pedantic -Wall -Wextra -c -o
OBJ = main.o analyzer.o
EXE = uBasicInterpreter

all: $(EXE)
$(EXE) : $(OBJ)
	gcc -o $(EXE) $(OBJ)

main.o : main.c
	gcc $(CFLAGS) main.o main.c

analyzer.o : analyzer.c
	gcc $(CFLAGS) analyzer.o analyzer.c