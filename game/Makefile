INC := -I../vendor -I/opt/homebrew/include
LIBS := -framework OpenGL

bin/game.dylib: main.cpp
	$(CXX) main.cpp -std=c++14 -dynamiclib -o bin/game.dylib -ldl -Wall -Wextra $(INC) $(LIBS)