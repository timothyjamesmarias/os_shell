shell: shell.o 
	gcc shell.o -o shell

shell.o: shell.c shell.h
	gcc --std=gnu99 -ggdb -c shell.c

clean:
	rm *.o shell
