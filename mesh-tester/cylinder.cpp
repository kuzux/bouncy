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

        faces.push_back({i+1, 1, i+2});
    }
    // for the final face, we need to do vertex k, center, vertex 1
    faces.push_back({k+1, 1, 2});

    // for the second circle (the top circle), everything is offset by k+1
    int offset = k+1;
    for(int i=1; i<k; i++) {
        faces.push_back({offset+i+1, offset+1, offset+i+2});
    }
    // for the final face, we need to do vertex k, center, vertex 1
    faces.push_back({offset+k+1, offset+1, offset+2});

    // for the sides, we need to add bottom i+1, top i+1, bottom i
    // then top i, bottom i, top i+1
    for(int i=1; i<k; i++) {
        faces.push_back({i+2, offset+i+1, i+1});
        faces.push_back({offset+i+1, i+2, offset+i+2});
    }

    // for the last side, instead of i and i+1, use k and 1
    faces.push_back({2, offset+k+1, k+1});
    faces.push_back({offset+k+1, 2, offset+2});
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
