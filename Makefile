all: alloctest

alloctest: alloctest.c
	gcc -g -std=c99 -o alloctest alloctest.c
