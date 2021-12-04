all: control.o game.o
	gcc -o control control.o
	gcc -o game game.o

control.o: control.c stuff.h
	gcc -c control.c

game.o: game.c stuff.h
	gcc -c game.c

clean:
	rm *.o
	rm control game
	rm gamefile.txt

ctrl: all
	./control

game: all
	./game
