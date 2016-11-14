ALL: main

mvar.o: src/mvar.c src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g

main: main.c mvar.o
	gcc -std=c99 $^ -o $@ -Wall -Wextra -g -lpthread

.PHONY: clean
clean:
	rm -f *.o main
