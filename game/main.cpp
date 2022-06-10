#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
using namespace std;

#define GL_GLEXT_PROTOTYPES 1
#define GL_SILENCE_DEPRECATION // make osx complain less
#include <OpenGL/gl3.h> // this should normally be GL/gl3.h
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>

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

extern "C" void Hello(const char*);
extern "C" int Initialize(bool, void*);
extern "C" void Update(KeyState, uint64_t);
extern "C" void Draw();
extern "C" void Cleanup();

struct GameState {
    glm::mat4 cameraTransform;

    glm::vec3 ballPosition;
    glm::vec3 ballVelocity;
    glm::vec3 ballForce;
};

// persisted game state
GameState* st;

struct Drawable {
    // store position / rotation etc. vectors in the state to calculaste / update this value
    glm::mat4 transform;
    vector<float> mesh;

    GLuint vao, vbo;
};

void generateDrawable(Drawable* d, const glm::mat4 transform, function<void(function<void(float)>)> generator) {
    *d = { transform, vector<float>(), 0, 0 };
    generator([&](float f){ d->mesh.push_back(f); });
    
    glGenVertexArrays(1, &d->vao);
    glBindVertexArray(d->vao);
    glGenBuffers(1, &d->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*d->mesh.size(), &d->mesh[0], GL_STATIC_DRAW);
}

void deleteDrawable(Drawable* d) {
    glDeleteBuffers(1, &d->vbo);
    glDeleteVertexArrays(1, &d->vao);
}

// temporaries - regenerated in initialize
Drawable cylinder;
Drawable ball;
GLuint program;

glm::mat4 cylinderTransform;
glm::mat4 projection;

// pass the view-projection matrix to draw it
void drawDrawable(const Drawable* d, glm::mat4 vp) {
    glm::mat4 mvp = vp * (d->transform);
    GLuint mvpLocation = glGetUniformLocation(program, "MVP");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);

    int numVertices = d->mesh.size() / 3;

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, d->vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    glDisableVertexAttribArray(0);
}

#define PI 3.141592f

#define ADD_FACE(i, j, k) { \
    addItem(vertices[(i)].x); \
    addItem(vertices[(i)].y); \
    addItem(vertices[(i)].z); \
    addItem(vertices[(j)].x); \
    addItem(vertices[(j)].y); \
    addItem(vertices[(j)].z); \
    addItem(vertices[(k)].x); \
    addItem(vertices[(k)].y); \
    addItem(vertices[(k)].z); \
}

void generateCylinder(function<void(float)> addItem) {
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
}

void generateSphere(function<void(float)> addItem) {
    vector<glm::vec3> vertices;

    // how many vertices in a circle
    int k = 40;

    // the sphere has k "levels" - each level corresponds to a y value (between 1 and -1)
    float delta_y = 2.0f / k;
    float y = 1.0f;
    for(int i=0; i<k; i++) {
        float r = sqrtf(1.0f - y * y);

        // each level then contains a circle
        float theta = 0.0f;
        float delta_th = 2 * PI / k;
        for(int i=0; i<k; i++) {
            float x = r*cos(theta);
            float z = r*sin(theta);
            vertices.push_back({x, y, z});

            theta += delta_th;
        }

        y -= delta_y;
    }

    // to get vertex (i, j) we need to do i*k+j (+1 for 1 based index)
    #define IDX0(i, j) ((i)*k+(j))

    // for each level i >= 1, face j >= 1
    // we connect (i, j) - (i-1, j) - (i, j-1)
    // and (i-1, j-1) - (i, j-1) - (i-1, j)
    for(int i=1; i<k; i++) {
        for(int j=1; j<k; j++) {
            ADD_FACE(IDX0(i, j), IDX0(i-1, j), IDX0(i, j-1));
            ADD_FACE(IDX0(i-1, j-1), IDX0(i, j-1), IDX0(i-1, j));
        }

        // add the final face, use j-1 = k-1, j=0
        ADD_FACE(IDX0(i, 0), IDX0(i-1, 0), IDX0(i, k-1));
        ADD_FACE(IDX0(i-1, k-1), IDX0(i, k-1), IDX0(i-1, 0));
    }

    // add a 2d circle for closing the final layer (use i=k-1)
    vertices.push_back({ 0.0f, -1.0f, 0.0f }); // center of the circle, has index k*k
    int final_center = k*k;

    for(int j=1; j<k; j++) {
        // for each face in the center, what you do is take point i, center, point i+1
        // to get a counter clockwise winding order

        ADD_FACE(IDX0(k-1, j-1), final_center, IDX0(k-1, j));
    }
    ADD_FACE(IDX0(k-1, k-1), final_center, IDX0(k-1, 0));

    #undef IDX0
}

#undef ADD_FACE

glm::mat4 ballTransformFromState() {
    return glm::scale(glm::translate(glm::mat4(1.f), st->ballPosition), glm::vec3(.3f));
}

int Initialize(bool reinit, void* state_) {
    st = (GameState*)state_;

    cylinderTransform = glm::mat4(1.f);
    projection = glm::perspective(glm::radians(70.0f), 4.0f / 3.0f, 0.1f, 100.f);
    if(!reinit) {
        st->cameraTransform = glm::lookAt(glm::vec3(4.f, 3.f, 3.f), 
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f));
        
        st->ballPosition = glm::vec3(2.f, .3f, 2.f);
        st->ballVelocity = glm::vec3(0.f);
        st->ballForce = glm::vec3(0.f, 10.f, 0.f);
    }

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    generateDrawable(&cylinder, cylinderTransform, generateCylinder);
    generateDrawable(&ball, ballTransformFromState(), generateSphere);

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
            st->cameraTransform = glm::rotate(st->cameraTransform, dt, glm::normalize(glm::vec3(x, y, z)));
        } else {
            st->cameraTransform = glm::translate(st->cameraTransform, dt*glm::vec3(x, y, z));
        }
    }

    float ballMass = 2.f;

    st->ballVelocity += (st->ballForce / ballMass) * dt;
    st->ballPosition += st->ballVelocity * dt;

    st->ballForce += ballMass * glm::vec3(0.f, -9.8f, 0.f) * dt; // gravity

    printf("ball y %f vy %f\n", st->ballPosition.y, st->ballVelocity.y);
    if(st->ballPosition.y < .3f) {
        // let it bounce
        st->ballForce = glm::vec3(0.f, 10.f, 0.f);
        st->ballVelocity = glm::vec3(0.f);
        st->ballPosition = glm::vec3(2.f, .3f, 2.f);
    }
    ball.transform = ballTransformFromState();
}

void Draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 vp = projection * st->cameraTransform;

    glUseProgram(program);
    drawDrawable(&cylinder, vp);
    drawDrawable(&ball, vp);
}

void Cleanup() {
    deleteDrawable(&cylinder);
    deleteDrawable(&ball);
    glDeleteProgram(program);
}

void Hello(const char* name) {
    printf("Hello, %s\n", name);
}
