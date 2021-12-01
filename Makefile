synthmake: main.c lib include
	gcc -o synth.exe main.c -I include/ -L lib -lraylib
	./synth.exe