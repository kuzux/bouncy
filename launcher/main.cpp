#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

#include <stdio.h>
#include <assert.h>

#define PI 3.141592f

GLuint vao;
GLuint vbo;
GLuint program;

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;

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

vector<float> cylinder;

void generateShape() {
    #define ADD_FACE(i, j, k) { \
        cylinder.push_back(vertices[(i)].x); \
        cylinder.push_back(vertices[(i)].y); \
        cylinder.push_back(vertices[(i)].z); \
        cylinder.push_back(vertices[(j)].x); \
        cylinder.push_back(vertices[(j)].y); \
        cylinder.push_back(vertices[(j)].z); \
        cylinder.push_back(vertices[(k)].x); \
        cylinder.push_back(vertices[(k)].y); \
        cylinder.push_back(vertices[(k)].z); \
    }

    vector<glm::vec3> vertices;
    // how many vertices in a circle
    int k = 40;

    float h = 10.0f;

    float delta = 2*PI / k;
    
    vertices.push_back({ 0.0f, 0.0f, 0.0f }); // center
    float rads = 0.0;
    for(int i=0; i<k; i++) {
        float x = cos(rads);
        float z = sin(rads);
        vertices.push_back({x, 0.0f, z});

        rads += delta;
    }

    // the second circle
    vertices.push_back({ 0.0f, h, 0.0f }); // center
    rads = 0.0;
    for(int i=0; i<k; i++) {
        float x = cos(rads);
        float z = sin(rads);
        vertices.push_back({x, h, z});

        rads += delta;
    }

    // index 0 is center
    // index 1 - (k-1) is all the vertices minus the last one

    for(int i=1; i<k; i++) {
        // for each face in the center, what you do is take point i, center, point i+1
        // to get a counter clockwise winding order

        ADD_FACE(i, 0, i+1);
    }
    // for the final face, we need to do vertex k, center, vertex 1
    ADD_FACE(k, 0, 1);

    // for the second circle (the top circle), everything is offset by k+1
    int offset = k+1;
    for(int i=1; i<k; i++) {
        ADD_FACE(offset+i, offset+0, offset+i+1);
    }
    // for the final face, we need to do vertex k, center, vertex 1
    ADD_FACE(offset+k, offset+0, offset+1);

    // for the sides, we need to add bottom i+1, top i+1, bottom i
    // then top i, bottom i, top i+1
    for(int i=1; i<k; i++) {
        ADD_FACE(i+1, offset+i+1, i);
        ADD_FACE(offset+i, i, offset+i+1);
    }

    // for the last side, instead of i and i+1, use k and 1
    ADD_FACE(2, offset+k, k);
    ADD_FACE(offset+k, 1, offset+1);

    #undef ADD_FACE
}

int Initialize() {
    generateShape();
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*cylinder.size(), &cylinder[0], GL_STATIC_DRAW);

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

// the delta t is in milliseconds
void Update(KeyState keys, uint64_t dt_ms) {
    float x = 0.f, y = 0.f, z = 0.f;
    float dt = dt_ms / 1000.f;
    if(keys.dirs.down) z -= 1.f;
    if(keys.dirs.up) z += 1.f;
    if(keys.dirs.left) x -= 1.f;
    if(keys.dirs.right) x += 1.f;
    if(keys.buttons[1]) y -= 1.f;
    if(keys.buttons[2]) y += 1.f;

    if(x != 0 || y != 0 || z != 0) {
        if(keys.buttons[0]) {
            view = glm::rotate(view, dt, glm::normalize(glm::vec3(x, y, z)));
        } else {
            view = glm::translate(view, dt*glm::vec3(x, y, z));
        }
    }
}

void Draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 mvp = projection * view * model;

    glUseProgram(program);

    GLuint mvpLocation = glGetUniformLocation(program, "MVP");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);

    int num_vertices = cylinder.size() / 3;

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
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

    KeyState keys = {};
    bool running = true;
    uint64_t ticks = SDL_GetTicks64();
    while(running) {
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
        Update(keys, newticks - ticks);
        ticks = newticks;
        Draw();
        SDL_GL_SwapWindow(win);
    }

    Cleanup();

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
