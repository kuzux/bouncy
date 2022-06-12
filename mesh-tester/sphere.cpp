#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
using namespace std;

#define PI 3.141592f

struct vec3f {
    float x, y, z;
};

struct vec3i {
    int x, y, z;
};

vector<vec3f> vertices;
vector<vec3i> faces;

void generate() {
    #define ADD_FACE(i, j, k) faces.push_back({i+1, j+1, k+1})

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
        // to get a counter clockwise winding order

        ADD_FACE(IDX0(k-1, j-1), final_center, IDX0(k-1, j));
    }
    ADD_FACE(IDX0(k-1, k-1), final_center, IDX0(k-1, 0));

    #undef IDX0
    #undef ADD_FACE
}

int main() {
    generate();

    cout << fixed << showpoint;
    cout << setprecision(4);

    for(vec3f v : vertices) {
        cout << "v " << v.x << " " << v.y << " " << v.z << endl;
    }

    for(vec3i f : faces) {
        cout << "f " << f.x << " " << f.y << " " << f.z << endl;
    }
}
