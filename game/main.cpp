#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
using namespace std;

#define BACKWARD_HAS_BFD 1
#include <backward.hpp>

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

extern "C" int Initialize(bool, void*);
extern "C" void Update(KeyState, uint64_t);
extern "C" void Draw();
extern "C" void Cleanup();

struct GameState {
    float cameraHeight;

    glm::vec3 ballPosition;
    glm::vec3 ballVelocity;
    glm::vec3 ballForce;
};

// persisted game state
GameState* st;

struct Mesh {
    vector<float> data;
    // TODO: Store vertices in a separate array and use an element array buffer
    GLuint vao, vbo;
};

struct Drawable {
    // store position / rotation etc. vectors in the state to calculaste / update this value
    glm::mat4 transform;
    // stored separately to allow for instancing
    const Mesh* mesh;
    // rgb value
    glm::vec3 color;
};

// temporaries - regenerated in initialize
Mesh cylinderMesh;
Mesh sphereMesh;
Mesh platformMesh;

Drawable cylinder;
Drawable ball;
vector<Drawable> platformSections;
GLuint program;

glm::mat4 cylinderTransform;
glm::mat4 view;
glm::mat4 projection;

void generateMesh(Mesh* m, function<void(function<void(float)>)> generator) {
    *m = { vector<float>(), 0, 0 };
    generator([&](float f){ m->data.push_back(f); });

    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);
    glGenBuffers(1, &m->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m->data.size(), &m->data[0], GL_STATIC_DRAW);
}

void deleteMesh(Mesh* m) {
    glDeleteBuffers(1, &m->vbo);
    glDeleteVertexArrays(1, &m->vao);
}

// pass the view-projection matrix to draw it
void drawDrawable(const Drawable* d, glm::mat4 vp) {
    glm::mat4 mvp = vp * (d->transform);
    GLuint mvpLocation = glGetUniformLocation(program, "MVP");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvp[0][0]);

    GLuint colorLocation = glGetUniformLocation(program, "ObjectColor");
    glUniform3fv(colorLocation, 1, &d->color[0]);

    int numVertices = d->mesh->data.size() / 3;

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, d->mesh->vao);
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

    float h = 12.0f;

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
        // for each face in the center, what you do is take point i+1, center, point i
        // to get a counter-clockwise winding order
        // (for the bottom face, the top face is reversed)

        ADD_FACE(i+1, 0, i);
    }
    // for the final face, we need to do vertex k, center, vertex 1
    ADD_FACE(1, 0, k);

    // for the second circle (the top circle), everything is offset by k+1
    int offset = k+1;
    for(int i=1; i<k; i++) {
        ADD_FACE(offset+i, offset+0, offset+i+1);
    }
    // for the final face, we need to do vertex k, center, vertex 1
    ADD_FACE(offset+k, offset+0, offset+1);

    // for the sides, we need to add bottom i, top i+1, bottom i+1
    // then top i+1, bottom i, top i
    for(int i=1; i<k; i++) {
        ADD_FACE(i, offset+i+1, i+1);
        ADD_FACE(offset+i+1, i, offset+i);
    }

    // for the last side, instead of i and i+1, use k and 1
    ADD_FACE(k, offset+k, 1);
    ADD_FACE(offset+1, 1, offset+k);
}

void generateSphere(function<void(float)> addItem) {
    vector<glm::vec3> vertices;

    // how many vertices in a circle
    int k = 40;

    // the sphere has k "levels"
    // the sphere's "levels" are denoted by an angle phi, between pi/2 and -pi/2

    float delta_phi = PI / k;
    float phi = PI / 2;
    for(int i=0; i<k; i++) {
        float y = sin(phi);
        float r = cos(phi);

        // each level then contains a circle
        float theta = 0.0f;
        float delta_th = 2 * PI / k;
        for(int i=0; i<k; i++) {
            float x = r*cos(theta);
            float z = r*sin(theta);
            vertices.push_back({x, y, z});

            theta += delta_th;
        }

        phi -= delta_phi;
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
        // to get a clockwise winding order

        ADD_FACE(IDX0(k-1, j-1), final_center, IDX0(k-1, j));
    }
    ADD_FACE(IDX0(k-1, k-1), final_center, IDX0(k-1, 0));

    #undef IDX0
}

void generatePlatformSection(function<void(float)> addItem) {
    vector<glm::vec3> vertices;

    // how many vertices in an arc
    int k = 5;

    float h = .1f;
    // inner & outer radii
    float r1 = 1.f;
    float r2 = 2.f;

    // each platform has 32 segments
    float delta = (2*PI/32) / (k-1);
    // the angle of each line within the arc is actually 1/(k-1) times the total length since 
    // one of the points is supposed to interact with the next one

    // vertices consist of 4 "rings"
    // vertex 0 .. k-1 => bottom inner
    // vertex k .. 2k-1 => bottom outer
    // vertex 2k .. 3k - 1 => top inner
    // vertex 3k .. 4k-1 => top outer
    
    float rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r1*cos(rads);
        float z = r1*sin(rads);
        vertices.push_back({x, 0.f, z});

        rads += delta;
    }

    rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r2*cos(rads);
        float z = r2*sin(rads);
        vertices.push_back({x, 0.f, z});

        rads += delta;
    }

    rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r1*cos(rads);
        float z = r1*sin(rads);
        vertices.push_back({x, h, z});

        rads += delta;
    }

    rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r2*cos(rads);
        float z = r2*sin(rads);
        vertices.push_back({x, h, z});

        rads += delta;
    }

    // bottom face
    for(int i=0; i<k-1; i++) {
        ADD_FACE(i, k+i, i+1);
        ADD_FACE(i+1, k+i, k+i+1);
    }

    // top face
    for(int i=0; i<k-1; i++) {
        ADD_FACE(2*k+i+1, 3*k+i, 2*k+i);
        ADD_FACE(3*k+i+1, 3*k+i, 2*k+i+1);
    }

    // side faces similar
    // bottom i, top i, bottom i+1
    // bottom i+1, top i, top i+1
    for(int i=0; i<k-1; i++) {
        // inner ring
        // the winding is reverse, since the "inside" face is front-facing
        ADD_FACE(i+1, 2*k+i, i);
        ADD_FACE(2*k+i+1, 2*k+i, i+1);
        // outer ring
        ADD_FACE(k+i, 3*k+i, k+i+1);
        ADD_FACE(k+i+1, 3*k+i, 3*k+i+1);
    }

    // side with i=0
    // top outer - bottom outer - bottom inner
    // top inner - top outer - bottom inner
    ADD_FACE(3*k, k, 0);
    ADD_FACE(2*k, 3*k, 0);

    // similarly with i=k-1
    // but the winding is reversed again
    ADD_FACE(k-1, k+k-1, 3*k+k-1);
    ADD_FACE(k-1, 3*k+k-1, 2*k+k-1);
}

#undef ADD_FACE

void generatePlatforms() {
    float theta = 0.0;
    float delta = 2*PI / 32;
    glm::vec3 axis(0.f, 1.f, 0.f);

    glm::vec3 blue(0.f, 0.f, 1.f);

    int numLevels = 5;
    float levelHeight = 2.f;
    float height = 0.f;
    for(int l=0; l<numLevels; l++) {
        glm::mat4 base = glm::translate(cylinderTransform, glm::vec3(0.f, height, 0.f));
        for(int i=0; i<32; i++) {
            if(i%3 != 0) platformSections.push_back({ glm::rotate(base, theta, axis), &platformMesh, blue });
            theta += delta;
        }
        height += levelHeight;
    }
}

glm::mat4 ballTransformFromState() {
    return glm::scale(glm::translate(glm::mat4(1.f), st->ballPosition), glm::vec3(.3f));
}

glm::mat4 cameraTransformFromState() {
    return glm::lookAt(
        glm::vec3(0.f, st->cameraHeight, 5.f),
        glm::vec3(0.f, st->cameraHeight-0.5f, 0.f),
        glm::vec3(0.f, 1.f, 0.f)
    );
}

int Initialize(bool reinit, void* state_) {
    backward::SignalHandling sh;
    st = (GameState*)state_;

    cylinderTransform = glm::mat4(1.f);
    projection = glm::perspective(glm::radians(70.0f), 4.0f / 3.0f, 0.1f, 100.f);
    if(!reinit) {
        st->ballPosition = glm::vec3(0.f, 11.f, 1.f);
        st->ballVelocity = glm::vec3(0.f);
        st->ballForce = glm::vec3(0.f, 10.f, 0.f);

        st->cameraHeight = st->ballPosition.y;
    }

    view = cameraTransformFromState();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.1f, 0.0f, 0.0f);
    glm::vec3 red(1.f, 0.f, 0.f);
    glm::vec3 blue(0.f, 0.f, 1.f);

    generateMesh(&cylinderMesh, generateCylinder);
    generateMesh(&sphereMesh, generateSphere);
    generateMesh(&platformMesh, generatePlatformSection);
    cylinder = { cylinderTransform, &cylinderMesh, blue };
    ball = { ballTransformFromState(), &sphereMesh, red };
    generatePlatforms();

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

    /*if(x != 0 || y != 0 || z != 0) {
        if(keys.buttons[0]) {
            st->cameraTransform = glm::rotate(st->cameraTransform, dt, glm::normalize(glm::vec3(x, y, z)));
        } else {
            st->cameraTransform = glm::translate(st->cameraTransform, dt*glm::vec3(x, y, z));
        }
    }*/

    float ballMass = 2.f;

    st->ballVelocity += (st->ballForce / ballMass) * dt;
    st->ballPosition += st->ballVelocity * dt;

    st->ballForce += ballMass * glm::vec3(0.f, -9.8f, 0.f) * dt; // gravity

    // printf("ball y %f vy %f\n", st->ballPosition.y, st->ballVelocity.y);
    if(st->ballPosition.y < .3f) {
        // let it bounce
        st->ballForce = glm::vec3(0.f, 10.f, 0.f);
        st->ballVelocity = glm::vec3(0.f);
        st->ballPosition.y = .3f;
    }

    if(st->ballPosition.y <= st->cameraHeight - 1.f) {
        // let camera follow the falling ball
        st->cameraHeight = st->ballPosition.y + 1.f;
    }

    ball.transform = ballTransformFromState();
    view = cameraTransformFromState();
}

void Draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 vp = projection * view;

    glUseProgram(program);
    drawDrawable(&cylinder, vp);
    drawDrawable(&ball, vp);
    for(auto& ps : platformSections) {
        drawDrawable(&ps, vp);
    }
}

void Cleanup() {
    deleteMesh(&cylinderMesh);
    deleteMesh(&sphereMesh);
    glDeleteProgram(program);
}
