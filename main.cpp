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

const float noise_amplitude = 1.0;

template <typename T> inline T lerp(const T &v0, const T &v1, float t) {
    return v0 + (v1-v0)*std::max(0.f, std::min(1.f, t));
}

//the perlin-noise generation code looks a bit odd with a lot of magic numbers floating around and there are no
//extra explainations of the algorithm, therefore its remain as a black box for me.
float hash(const float n) {
    float x = sin(n)*43758.5453f;
    return x-floor(x);
}

float noise(const Vec3f &x) {
    Vec3f p(floor(x.x), floor(x.y), floor(x.z));
    Vec3f f(x.x-p.x, x.y-p.y, x.z-p.z);
    f = f*(f*(Vec3f(3.f, 3.f, 3.f)-f*2.f));
    float n = p*Vec3f(1.f, 57.f, 113.f);
    return lerp(lerp(
                     lerp(hash(n +  0.f), hash(n +  1.f), f.x),
                     lerp(hash(n + 57.f), hash(n + 58.f), f.x), f.y),
                lerp(
                    lerp(hash(n + 113.f), hash(n + 114.f), f.x),
                    lerp(hash(n + 170.f), hash(n + 171.f), f.x), f.y), f.z);
}

Vec3f rotate(const Vec3f &v) {
    return Vec3f(Vec3f(0.00,  0.80,  0.60)*v, Vec3f(-0.80,  0.36, -0.48)*v, Vec3f(-0.60, -0.48,  0.64)*v);
}

float fractal_brownian_motion(const Vec3f &x) {
    Vec3f p = rotate(x);
    float f = 0;
    f += 0.5000*noise(p); p = p*2.32;
    f += 0.2500*noise(p); p = p*3.03;
    f += 0.1250*noise(p); p = p*2.61;
    f += 0.0625*noise(p);
    return f/0.9375;
}

float signed_distance_sphere(const Vec3f& p, const Vec3f& center, float radius) {
    // Vec3f s = Vec3f(p-center).normalize(radius);// the "hit" point
    return (p-center).norm() - (radius - fractal_brownian_motion(p*3.4));
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
    // if (orig*orig - pow(orig*dir, 2) > pow(RADIUS, 2)) return false; // early discard
    hit = orig;
    for (int i = 0; i < 128; ++i) {
        float d = signed_distance_sphere(hit, CENTER, RADIUS);
        if (d < 0) return true;
        hit = hit + dir * std::max(0.01f, d*0.1f);
    }
    return false;
}

Vec3f palette_fire(const float d) {
    const Vec3f   yellow(1.7, 1.3, 1.0); // note that the color is "hot", i.e. has components >1
    const Vec3f   orange(1.0, 0.6, 0.0);
    const Vec3f      red(1.0, 0.0, 0.0);
    const Vec3f darkgray(0.2, 0.2, 0.2);
    const Vec3f     gray(0.4, 0.4, 0.4);

    float x = std::max(0.f, std::min(1.f, d));
    if (x<.25f)
        return lerp(gray, darkgray, x*4.f);
    else if (x<.5f)
        return lerp(darkgray, red, x*4.f-1.f);
    else if (x<.75f)
        return lerp(red, orange, x*4.f-2.f);
    return lerp(orange, yellow, x*4.f-3.f);
}

int main() {
    const int width = 640;
    const int height = 480;
    const float fov = M_PI/3.0f;//fovy
    srand(time(0));
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
                float noise_level = (RADIUS-(hit-CENTER).norm());
                auto n = distance_field_normal(hit);
                auto l = (light - hit).normalize();
                float intensity = std::max(n*l, 0.4f) * 1.0;
                framebuffer[i+j*width] = palette_fire(1.3f*noise_level/RADIUS) * intensity;
            } else {
                framebuffer[i+j*width] = Vec3f{0.2, 0.7, 0.8};
            }
        }
    }

    write_ppm("./out.ppm", framebuffer, width, height);

    return 0;
}