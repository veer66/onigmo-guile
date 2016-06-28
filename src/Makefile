all: main.c
	gcc `pkg-config --cflags guile-2.0` -shared -o libguile-onigmo.so -fPIC main.c -lonig
