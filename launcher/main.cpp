#include <iostream>
#include <dlfcn.h>
#include <sys/stat.h>
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
using namespace std;

#include <stdio.h>
#include <assert.h>

struct KeyState {
    // virtual game pad with 2 analogs, a d-pad and 16 "regular" buttons

    float a1x, a1y;
    float a2x, a2y;

    union {
        bool elements[4];
        struct {
            bool up, down, left, right;
        };
    } dirs;

    bool buttons[16];
};

void* gamelib = NULL;
int (*_Initialize)(bool, void*);
void (*_Update)(KeyState, uint64_t);
void (*_Draw)();
void (*_Cleanup)();

int ReloadGamelib(const char* path) {
    if(gamelib) {
        _Cleanup();
        dlclose(gamelib);
    }

    gamelib = dlopen(path, 0);
    if(!gamelib) {
        cerr << "Could not open the shared library " << path << "\n";
        return 1;
    }

    _Initialize = (int (*)(bool, void*))dlsym(gamelib, "Initialize");
    _Update = (void (*)(KeyState, uint64_t))dlsym(gamelib, "Update");
    _Draw = (void (*)())dlsym(gamelib, "Draw");
    _Cleanup = (void (*)())dlsym(gamelib, "Cleanup");

    if(!_Initialize || !_Update || !_Draw || !_Cleanup) {
        cerr << "Could not load functions from the shared library " << path << "\n";
        return 1;
    }

    return 0;
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetSwapInterval(1); // turn on vsync

    SDL_Window* win = win = SDL_CreateWindow("sdl opengl thingy", 
        0, 0,
        800, 600, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
    assert(win);

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, ctx);

    int rc;
    rc = ReloadGamelib(argv[1]);
    if(rc) return rc;

    void* gameState = (void*)(new uint8_t[1 << 27]);
    memset(gameState, 0, 1 << 27);
    rc = _Initialize(false, gameState);
    if(rc) return rc;

    struct stat fileInfo;
    stat(argv[1], &fileInfo);
    // I think the m_timespec thing is osx-specific
    time_t lastModified = fileInfo.st_mtimespec.tv_sec;

    glViewport(0, 0, 800, 600);

    KeyState keys = {};
    bool running = true;
    uint64_t ticks = SDL_GetTicks64();

    while(running) {
        stat(argv[1], &fileInfo);
        if(fileInfo.st_mtimespec.tv_sec > lastModified) {
            rc = ReloadGamelib(argv[1]);
            lastModified = fileInfo.st_mtimespec.tv_sec;
            if(rc) return rc;
            rc = _Initialize(true, gameState);
            if(rc) return rc;
        }

        SDL_Event e;
        // we need to drain the event queue on each "frame" to be
        // displayed properly on osx
        // and yes, we need to draw multiple times to actually get a 
        // display (due to triple buffering, I guess)
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;

            // TODO: Handle the case where the key was pressed and released more
            // than once within a single frame
            if(e.type == SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                case SDLK_UP: 
                    keys.dirs.up = true;
                    break;
                case SDLK_DOWN: 
                    keys.dirs.down = true;
                    break;
                case SDLK_LEFT: 
                    keys.dirs.left = true;
                    break;
                case SDLK_RIGHT: 
                    keys.dirs.right = true;
                    break;
                case SDLK_LSHIFT:
                    keys.buttons[0] = true;
                    break;
                case SDLK_a:
                    keys.buttons[1] = true;
                    break;
                case SDLK_q:
                    keys.buttons[2] = true;
                    break;
                }
            } else if(e.type == SDL_KEYUP) {
                switch(e.key.keysym.sym) {
                case SDLK_UP: 
                    keys.dirs.up = false;
                    break;
                case SDLK_DOWN: 
                    keys.dirs.down = false;
                    break;
                case SDLK_LEFT: 
                    keys.dirs.left = false;
                    break;
                case SDLK_RIGHT: 
                    keys.dirs.right = false;
                    break;
                case SDLK_LSHIFT:
                    keys.buttons[0] = false;
                    break;
                case SDLK_a:
                    keys.buttons[1] = false;
                    break;
                case SDLK_q:
                    keys.buttons[2] = false;
                    break;
                }
            }
        }

        // I'm not totally sure about it but the delta t calculation might not be the most accurate
        uint64_t newticks = SDL_GetTicks64();
        _Update(keys, newticks - ticks);
        ticks = newticks;
        _Draw();
        SDL_GL_SwapWindow(win);
    }

    _Cleanup();

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
