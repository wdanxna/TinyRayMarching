#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

Vec3f CENTER = Vec3f{0, 0, 0};
float RADIUS = 1.5f;

void write_ppm(const char* filename, const std::vector<Vec3f>& data, int width, int height) {
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            ofs << (char)(std::max(0, std::min(255, static_cast<int>(255 * data[i][j]))));
        }
    }
}

float signed_distance_sphere(const Vec3f& p, const Vec3f& center, float radius) {
    Vec3f s = Vec3f(p-center).normalize(radius);// the "hit" point
    return (p-center).norm() - (radius + (sin(16*s.x)*sin(16*s.y)*sin(16*s.z)*0.2f));
}

Vec3f distance_field_normal(const Vec3f& p) {
    //finite differences, the direction of change of f(x,y,z) at point p
    const float eps = 0.1f;
    float d = signed_distance_sphere(p, CENTER, RADIUS);
    float dx = signed_distance_sphere(p+Vec3f{eps, 0, 0}, CENTER, RADIUS) - d;
    float dy = signed_distance_sphere(p+Vec3f{0, eps, 0}, CENTER, RADIUS) - d;
    float dz = signed_distance_sphere(p+Vec3f{0, 0, eps}, CENTER, RADIUS) - d;
    return Vec3f{dx, dy, dz}.normalize();
}

bool sphere_trace(const Vec3f& orig, const Vec3f& dir, Vec3f& hit) {
    hit = orig;
    for (int i = 0; i < 128; ++i) {
        float d = signed_distance_sphere(hit, CENTER, RADIUS);
        if (d < 0) return true;
        hit = hit + dir * std::max(0.01f, d*0.1f);
    }
    return false;
}

int main() {
    const int width = 640;
    const int height = 480;
    const float fov = M_PI/3.0f;//fovy

    Vec3f light{10, 10, 10};//light
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
            float y = -((j) - height/2.0f);
            float z = -height / (2.0f * tanf(fov/2.0f));
            
            Vec3f hit;
            if (sphere_trace(Vec3f{0, 0, 3}, Vec3f{x, y, z}.normalize(), hit)) {
                auto n = distance_field_normal(hit);
                auto l = (light - hit).normalize();
                float intensity = std::max(n*l, 0.4f) * 1.0;
                framebuffer[i+j*width] = Vec3f{1,1,1} * intensity;
            } else {
                framebuffer[i+j*width] = Vec3f{0.2, 0.7, 0.8};
            }
        }
    }

    write_ppm("./out.ppm", framebuffer, width, height);

    return 0;
}