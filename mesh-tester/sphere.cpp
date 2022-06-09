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
    #define IDX1(i, j) ((i)*k+(j)+1)

    // for each level i >= 1, face j >= 1
    // we connect (i, j) - (i-1, j) - (i, j-1)
    // and (i-1, j-1) - (i, j-1) - (i-1, j)
    for(int i=1; i<k; i++) {
        for(int j=1; j<k; j++) {
            faces.push_back({ IDX1(i, j), IDX1(i-1, j), IDX1(i, j-1) });
            faces.push_back({ IDX1(i-1, j-1), IDX1(i, j-1), IDX1(i-1, j) });
        }

        // add the final face, use j-1 = k-1, j=0
        faces.push_back({ IDX1(i, 0), IDX1(i-1, 0), IDX1(i, k-1) });
        faces.push_back({ IDX1(i-1, k-1), IDX1(i, k-1), IDX1(i-1, 0) });
    }

    // add a 2d circle for closing the final layer (use i=k-1)
    vertices.push_back({ 0.0f, -1.0f, 0.0f }); // center of the circle, has index k*k
    int final_center = k*k+1;

    for(int j=1; j<k; j++) {
        // for each face in the center, what you do is take point i, center, point i+1
        // to get a counter clockwise winding order

        faces.push_back({IDX1(k-1, j-1), final_center, IDX1(k-1, j)});
    }
    faces.push_back({IDX1(k-1, k-1), final_center, IDX1(k-1, 0)});

    #undef IDX1
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
