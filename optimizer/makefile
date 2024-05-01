EXECUTABLE = execute_optimization
EXEC_SOURCE = execute.c
LLVMCODE_SOURCE = llvm_parser.c
OPTIMIZATIONS_SOURCE = optimizations.c

all: $(EXECUTABLE)

$(EXECUTABLE):
	g++ -Wall -Wextra -g $(shell llvm-config-15 --cflags) -I /usr/include/llvm-c-15/ -c $(EXEC_SOURCE) -o $(EXEC_SOURCE:.c=.o)
	g++ -Wall -Wextra -g $(shell llvm-config-15 --cflags) -I /usr/include/llvm-c-15/ -c $(LLVMCODE_SOURCE) -o $(LLVMCODE_SOURCE:.c=.o)
	g++ -Wall -Wextra -g $(shell llvm-config-15 --cflags) -I /usr/include/llvm-c-15/ -c $(OPTIMIZATIONS_SOURCE) -o $(OPTIMIZATIONS_SOURCE:.c=.o)
	g++ $(EXEC_SOURCE:.c=.o) $(LLVMCODE_SOURCE:.c=.o) $(OPTIMIZATIONS_SOURCE:.c=.o) $(shell llvm-config-15 --ldflags --libs core) -L/usr/lib/llvm-15/lib -lLLVM-15 -o $(EXECUTABLE)

clean:
	rm -rf $(EXECUTABLE) *.o

