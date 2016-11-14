ALL: main

mvar.o: src/mvar.c src/mvar.h
	gcc $< -c -o $@ -Wall -Wextra -g

main: main.c mvar.o
	gcc $^ -o $@ -Wall -Wextra -g -lpthread

.PHONY: clean
clean:
	rm -f *.o main
