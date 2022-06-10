INC := -I/opt/homebrew/include

bin/game.dylib: main.cpp
	$(CXX) main.cpp -std=c++14 -dynamiclib -o bin/game.dylib -Wall -Wextra $(INC) -framework OpenGL