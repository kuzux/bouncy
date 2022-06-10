INCLUDES := -I/opt/homebrew/include

bin/main: main.cpp
	$(CXX) main.cpp -std=c++14 -o bin/main -Wall -Wextra $(INCLUDES) `sdl2-config --cflags --libs` -framework OpenGL