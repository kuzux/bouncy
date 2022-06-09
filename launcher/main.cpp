#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>
#include <OpenGL/gl.h> // osx-specific, normally it is GL.gl.h

#include <assert.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* win = win = SDL_CreateWindow("sdl opengl thingy", 
        0, 0,
        800, 800, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
    assert(win);

    SDL_GLContext ctx = SDL_GL_CreateContext(win);

    glViewport(0, 0, 800, 800);

    bool running = true;
    while(running) {
        SDL_Event e;
        // we need to drain the event queue on each "frame" to be
        // displayed properly on osx
        // and yes, we need to draw multiple times to actually get a 
        // display (due to triple buffering, I guess)
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }

        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(win);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}