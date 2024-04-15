source = mini
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all
YFLAGS=-d --debug
CFLAGS = -Wall -Wextra -g -Wno-deprecated -std=c++11

// -DYYDEBUG=1

mini.out: $(source).l $(source).y ast.c semantic.o
	yacc -d -v $(source).y
	lex $(source).l
	g++ -g lex.yy.c y.tab.c ast.c semantic.o -o $(source).out

semantic.o: semantic.c semantic.h
	g++ $(CFLAGS) -c semantic.c

clean:
	rm lex.yy.c y.tab.c y.tab.h $(source).out y.output semantic.o