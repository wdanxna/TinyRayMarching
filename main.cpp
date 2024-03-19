#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

void write_ppm(const char* filename, const std::vector<Vec3f>& data, int width, int height) {
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            ofs << (char)(std::max(0, std::min(255, static_cast<int>(255 * data[i][j]))));
        }
    }
}

bool sphere_trace(const Vec3f& orig, const Vec3f& dir, Vec3f& hit) {
    return false;
}

int main() {
    const int width = 640;
    const int height = 480;
    const float fov = M_PI/3.0f;//fovy

    std::vector<Vec3f> framebuffer(width*height);

#pragma omp parallel for
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            //screen coordiates to world(view) coordinates as
            //if the projection plan is located at (x, y, z).
            //note that it is a different conversion style compares to
            //the the raytracer course, the later one assumes the projection plane
            //is located at z=0. In this course, the z of the projection plane has to be calculated
            //using fov and screen height.
            float x = (i) - width/2.0f;
            float y = (j) - height/2.0f;
            float z = -height / (2.0f * tanf(fov/2.0f));
            
            Vec3f hit;
            if (sphere_trace(Vec3f{0, 0, 3}, Vec3f{x, y, z}.normalize(), hit)) {
                framebuffer[i+j*width] = Vec3f{1,1,1};
            } else {
                framebuffer[i+j*width] = Vec3f{0.2, 0.7, 0.8};
            }
        }
    }

    write_ppm("./out.ppm", framebuffer, width, height);

    return 0;
}