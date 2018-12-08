shell: main.o shell.o
	gcc -o shell main.o shell.o -g

main.o: main.c shell.h
	gcc -c main.c -g

shell.o: shell.c shell.h
	gcc -c shell.c -g

clean:
	rm *.o shell
