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

    // how many vertices in a circle
    int k = 40;

    float h = 10.0f;

    float delta = 2*PI / k;

    // vertex 0 is bottom face center
    // vertex 1 .. k is bottom face edges
    // vertex k+1 is top face center
    // vertex k+2 .. 2k+1 is top face edges

    // normal 0 is bottom face
    // normals 1 .. k are outwards from the edges
    // normal k+1 is top face
    
    vertices.push_back({ 0.0f, 0.0f, 0.0f }); // center
    normals.push_back({ 0.f, -1.f, 0.f });
    float rads = 0.0;
    for(int i=0; i<k; i++) {
        float x = cos(rads);
        float z = sin(rads);
        vertices.push_back({x, 0.0f, z});
        normals.push_back({ x, 0.f, z });

        rads += delta;
    }

    normals.push_back({ 0.f, 1.f, 0.f });

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

    // the bottom face all has bottom face normals, obviously

    for(int i=1; i<k; i++) {
        // for each face in the center, what you do is take point i+1, center, point i
        // to get a counter-clockwise winding order
        // (for the bottom face, the top face is reversed)
        ADD_FACE(i+1, 0, i, 0, 0, 0);
    }
    // for the final face, we need to do vertex 1, center, vertex k
    ADD_FACE(1, 0, k, 0, 0, 0);

    // for the second circle (the top circle), everything is offset by k+1
    int offset = k+1;
    for(int i=1; i<k; i++) {
        ADD_FACE(offset+i, offset+0, offset+i+1, k+1, k+1, k+1);
    }
    // for the final face, we need to do vertex k, center, vertex 1
    ADD_FACE(offset+k, offset+0, offset+1, k+1, k+1, k+1);

    // for the sides, we need to add bottom i, top i+1, bottom i+1
    // then top i+1, bottom i, top i

    // for the side normals, vertex i has the normal i as well

    for(int i=1; i<k; i++) {
        ADD_FACE(i, offset+i, i+1, i, i, i+1);
        ADD_FACE(offset+i+1, i+1, offset+i, i+1, i+1, i);
    }

    // for the last side, instead of i and i+1, use k and 1
    ADD_FACE(k, offset+k, 1, k, k, 1);
    ADD_FACE(offset+1, 1, offset+k, 1, 1, k);

    #undef ADD_FACE
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
