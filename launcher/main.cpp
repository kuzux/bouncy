#include <iostream>
#include <fstream>
#include <sstream>
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

#include <stdio.h>
#include <assert.h>

static const float vertices[] = {
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   0.0f,  1.0f, 0.0f,
};

GLuint vao;
GLuint vbo;
GLuint program;

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;

int Initialize() {
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    projection = glm::perspective(glm::radians(70.0f), 4.0f / 3.0f, 0.1f, 100.f);
    view = glm::lookAt(glm::vec3(4.f, 3.f, 3.f), 
        glm::vec3(0.f, 0.f, 0.f),
        glm::vec3(0.f, 1.f, 0.f));
    model = glm::mat4(1.f);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    ifstream vertFile("vertex.gl");
    ifstream fragFile("fragment.gl");

    string vertSrc;
    string fragSrc;

    if(vertFile.is_open()) {
        stringstream sstr;
        sstr << vertFile.rdbuf();
        vertSrc = sstr.str();
        vertFile.close();
    } else {
        cerr << "Could not open vertex shader file\n";
        return 1;
    }

    if(fragFile.is_open()) {
        stringstream sstr;
        sstr << fragFile.rdbuf();
        fragSrc = sstr.str();
        fragFile.close();
    } else {
        cerr << "Could not open fragment shader file\n";
        return 1;
    }

    int rc;
    char const* vertCSrc = vertSrc.c_str();
    char const* fragCSrc = fragSrc.c_str();

    glShaderSource(vert, 1, &vertCSrc, NULL);
    glCompileShader(vert);

    glGetShaderiv(vert, GL_COMPILE_STATUS, &rc);
    if(rc != GL_TRUE) {
        int len;
        glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &len);
        char* msg = new char[len];
        glGetShaderInfoLog(vert, len, NULL, msg);
        cerr << "Vertex shader compile error:\n" << msg << "\n";
        delete[] msg;
        return 1;
    }

    glShaderSource(frag, 1, &fragCSrc, NULL);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &rc);
    if(rc != GL_TRUE) {
        int len;
        glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &len);
        char* msg = new char[len];
        glGetShaderInfoLog(frag, len, NULL, msg);
        cerr << "Fragment shader compile error:\n" << msg << "\n";
        delete[] msg;
        return 1;
    }

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &rc);
    if(rc != GL_TRUE) {
        int len;
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &len);
        char* msg = new char[len];
        glGetShaderInfoLog(frag, len, NULL, msg);
        cerr << "Shader link error:\n" << msg << "\n";
        delete[] msg;
        return 1;
    }

    glDetachShader(program, vert);
    glDetachShader(program, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return 0;
}

void Update() {

}

void Draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 mvp = projection * view * model;

    glUseProgram(program);

    GLuint mvpLocation = glGetUniformLocation(program, "MVP");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(0);
}

void Cleanup() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}

int main() {
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
    glViewport(0, 0, 800, 600);
    if((rc = Initialize())) {
        return rc;
    };

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

        Update();
        Draw();
        SDL_GL_SwapWindow(win);
    }

    Cleanup();

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
