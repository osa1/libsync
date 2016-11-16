ALL: main main_eventfd

mvar_sem.o: src/mvar.c src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g

mvar_eventfd.o: src/mvar.c src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g -DUSE_EVENTFD

chan_mvar.o: src/chan_mvar.c src/chan_mvar.h src/mvar.h
	gcc -std=c99 $< -c -o $@ -Wall -Wextra -g

main: main.c mvar_sem.o chan_mvar.o
	gcc -std=c99 $^ -o $@ -Wall -Wextra -g -lpthread

main_eventfd: main.c mvar_eventfd.o chan_mvar.o
	gcc -std=c99 $^ -o $@ -Wall -Wextra -g -lpthread -DUSE_EVENTFD

.PHONY: clean
clean:
	rm -f *.o main main_eventfd
