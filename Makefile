ALL: main

mvar.o: src/mvar.c src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g

chan_mvar.o: src/chan_mvar.c src/chan_mvar.h src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g

main: main.c mvar.o chan_mvar.o
	gcc -std=c99 $^ -o $@ -Wall -Wextra -g -lpthread

.PHONY: clean
clean:
	rm -f *.o main
