all:
	clang++ bap.cpp -O3 -o bapple `pkg-config --cflags --libs sdl2 SDL2_image` -std=c++11