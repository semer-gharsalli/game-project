prog:main.o
	gcc main.o -o prog -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -g
main.o:main.c
	gcc -c main.c -g


