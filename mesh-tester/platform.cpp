#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
using namespace std;

#define PI 3.141592f

struct vec3f {
    float x, y, z;
};

struct face {
    // vertex position indices
    int i, j, k;
    // vertex normal indices
    int ni, nj, nk;
};

vector<vec3f> vertices;
vector<vec3f> normals;
vector<face> faces;

void generate() {
    #define ADD_FACE(i, j, k, ni, nj, nk) faces.push_back({ i+1, j+1, k+1, ni+1, nj+1, nk+1 })

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
    // vertex 2k .. 3k-1 => top inner
    // vertex 3k .. 4k-1 => top outer

    // the normals are as follows:
    // normal 0 => bottom face
    // normal 1 => top face
    // normal 2 .. k+1 => inner face
    // normal k+2 .. 2k+1 => outer face
    // normal 2k+2 => side a (theta = 0)
    // normal 2k+3 => side b (theta != 0)

    normals.push_back({ 0.f, -1.f, 0.f });
    normals.push_back({ 0.f, 1.f, 0.f });
    
    float rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r1*cos(rads);
        float z = r1*sin(rads);
        vertices.push_back({x, 0.f, z});
        normals.push_back({-x, 0.f, -z});

        rads += delta;
    }

    rads = 0.f;
    for(int i=0; i<k; i++) {
        float x = r2*cos(rads);
        float z = r2*sin(rads);
        vertices.push_back({x, 0.f, z});
        normals.push_back({x, 0.f, z});

        rads += delta;
    }

    rads = 0.f;
    normals.push_back({ -1, 0, 0 });
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
    normals.push_back({ -cos(rads), 0, -sin(rads) });

    // bottom face
    for(int i=0; i<k-1; i++) {
        ADD_FACE(i, k+i, i+1, 0, 0, 0);
        ADD_FACE(i+1, k+i, k+i+1, 0, 0, 0);
    }

    // top face
    for(int i=0; i<k-1; i++) {
        ADD_FACE(2*k+i+1, 3*k+i, 2*k+i, 1, 1, 1);
        ADD_FACE(3*k+i+1, 3*k+i, 2*k+i+1, 1, 1, 1);
    }

    // side faces similar
    // bottom i, top i, bottom i+1
    // bottom i+1, top i, top i+1
    for(int i=0; i<k-1; i++) {
        // inner ring
        // the winding is reverse, since the "inside" face is front-facing
        ADD_FACE(i+1, 2*k+i, i, i+3, i+2, i+2);
        ADD_FACE(2*k+i+1, 2*k+i, i+1, i+3, i+2, i+3);
        // outer ring
        ADD_FACE(k+i, 3*k+i, k+i+1, k+i+2, k+i+2, k+i+3);
        ADD_FACE(k+i+1, 3*k+i, 3*k+i+1, k+i+3, k+i+2, k+i+3);
    }

    // side with i=0
    // top outer - bottom outer - bottom inner
    // top inner - top outer - bottom inner
    ADD_FACE(3*k, k, 0, 2*k+2, 2*k+2, 2*k+2);
    ADD_FACE(2*k, 3*k, 0, 2*k+2, 2*k+2, 2*k+2);

    // similarly with i=k-1
    // but the winding is reversed again
    ADD_FACE(k-1, k+k-1, 3*k+k-1, 2*k+3, 2*k+3, 2*k+3);
    ADD_FACE(k-1, 3*k+k-1, 2*k+k-1, 2*k+3, 2*k+3, 2*k+3);
}

int main() {
    generate();

    cout << fixed << showpoint;
    cout << setprecision(4);

    for(vec3f v : vertices) {
        cout << "v " << v.x << " " << v.y << " " << v.z << endl;
    }

    for(vec3f v : normals) {
        cout << "vn " << v.x << " " << v.y << " " << v.z << endl;
    }

    for(face f : faces) {
        cout << "f " << f.i << "//" << f.ni << " "
            << f.j << "//" << f.nj << " "
            << f.k << "//" << f.nk << "\n";
    }
}
