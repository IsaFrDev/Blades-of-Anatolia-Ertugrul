#pragma once
// Ertugrul :: minimal 3D math (tashqi kutubxonasiz)
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace ert {

constexpr float PI  = 3.14159265358979323846f;
constexpr float TAU = 6.28318530717958647692f;

inline float deg2rad(float d) { return d * (PI / 180.0f); }
inline float rad2deg(float r) { return r * (180.0f / PI); }
inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float saturate(float v) { return clampf(v, 0.0f, 1.0f); }
inline float smoothstepf(float t) { t = saturate(t); return t * t * (3.0f - 2.0f * t); }
inline float easeInOut(float t) { t = saturate(t); return t < 0.5f ? 2*t*t : 1.0f - 0.5f*(2.0f-2*t)*(2.0f-2*t); }
inline float easeOutCubic(float t){ t = saturate(t); float u = 1.0f - t; return 1.0f - u*u*u; }
// yaw farqini [-180,180] oralig'iga keltirish
inline float wrapAngleDeg(float d) { while (d < -180.0f) d += 360.0f; while (d > 180.0f) d -= 360.0f; return d; }
inline float lerpAngleDeg(float a, float b, float t) { return a + wrapAngleDeg(b - a) * saturate(t); }
// kadrga bog'liq bo'lmagan silliq yaqinlashish
inline float damp(float a, float b, float lambda, float dt) { return lerpf(a, b, 1.0f - std::exp(-lambda * dt)); }

struct Vec2 {
    float x = 0.0f, y = 0.0f;
    Vec2() = default;
    Vec2(float X, float Y) : x(X), y(Y) {}
};

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)       const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s)       const { return {x/s, y/s, z/s}; }
    Vec3 operator-()              const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o){ x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3& operator*=(float s)      { x*=s; y*=s; z*=s; return *this; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3  cross(const Vec3& a, const Vec3& b) { return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }
inline float lengthSq(const Vec3& v) { return dot(v, v); }
inline float distance(const Vec3& a, const Vec3& b) { return length(a - b); }
inline float distanceXZ(const Vec3& a, const Vec3& b) { float dx=a.x-b.x, dz=a.z-b.z; return std::sqrt(dx*dx+dz*dz); }
inline Vec3  normalize(const Vec3& v) { float l = length(v); return l > 1e-8f ? v / l : Vec3{0,0,0}; }
inline Vec3  lerp(const Vec3& a, const Vec3& b, float t) { return { lerpf(a.x,b.x,t), lerpf(a.y,b.y,t), lerpf(a.z,b.z,t) }; }
inline Vec3  dampV(const Vec3& a, const Vec3& b, float lambda, float dt) {
    float t = 1.0f - std::exp(-lambda * dt);
    return lerp(a, b, t);
}
// Catmull-Rom (kamera yo'llari uchun silliq egri)
inline Vec3 catmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) {
    float t2 = t*t, t3 = t2*t;
    return (p1*2.0f + (p2-p0)*t + (p0*2.0f - p1*5.0f + p2*4.0f - p3)*t2 + (p1*3.0f - p0 - p2*3.0f + p3)*t3) * 0.5f;
}
// yaw: +Z o'qidan soat strelkasi bo'yicha, gradusda
inline float yawFromDir(const Vec3& d) { return rad2deg(std::atan2(d.x, d.z)); }
inline Vec3  dirFromYaw(float yawDeg) { float r = deg2rad(yawDeg); return { std::sin(r), 0.0f, std::cos(r) }; }

// 4x4 ustun-tartibli matritsa (OpenGL bilan mos)
struct Mat4 {
    float m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

    static Mat4 identity() { return Mat4{}; }
    static Mat4 translate(const Vec3& t) { Mat4 r; r.m[12]=t.x; r.m[13]=t.y; r.m[14]=t.z; return r; }
    static Mat4 scale(const Vec3& s)     { Mat4 r; r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z; return r; }
    static Mat4 rotateX(float deg) { float c=std::cos(deg2rad(deg)), s=std::sin(deg2rad(deg)); Mat4 r; r.m[5]=c; r.m[6]=s; r.m[9]=-s; r.m[10]=c; return r; }
    static Mat4 rotateY(float deg) { float c=std::cos(deg2rad(deg)), s=std::sin(deg2rad(deg)); Mat4 r; r.m[0]=c; r.m[2]=-s; r.m[8]=s; r.m[10]=c; return r; }
    static Mat4 rotateZ(float deg) { float c=std::cos(deg2rad(deg)), s=std::sin(deg2rad(deg)); Mat4 r; r.m[0]=c; r.m[1]=s; r.m[4]=-s; r.m[5]=c; return r; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int rw = 0; rw < 4; ++rw) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) s += m[k*4 + rw] * o.m[c*4 + k];
                r.m[c*4 + rw] = s;
            }
        return r;
    }
    Vec3 transformPoint(const Vec3& p) const {
        return { m[0]*p.x + m[4]*p.y + m[8]*p.z  + m[12],
                 m[1]*p.x + m[5]*p.y + m[9]*p.z  + m[13],
                 m[2]*p.x + m[6]*p.y + m[10]*p.z + m[14] };
    }
    Vec3 transformDir(const Vec3& d) const {
        return { m[0]*d.x + m[4]*d.y + m[8]*d.z,
                 m[1]*d.x + m[5]*d.y + m[9]*d.z,
                 m[2]*d.x + m[6]*d.y + m[10]*d.z };
    }
};

// Determinatsiyalangan RNG (xarita generatsiyasi qayta ishga tushirilganda bir xil bo'lishi uchun)
class Rng {
public:
    explicit Rng(uint32_t seed = 1u) : s_(seed ? seed : 1u) {}
    uint32_t next() { s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5; return s_; }
    float    nextFloat() { return (next() >> 8) * (1.0f / 16777216.0f); }         // [0,1)
    float    range(float a, float b) { return a + nextFloat() * (b - a); }
    int      rangeI(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); }
private:
    uint32_t s_;
};

// Qiymat shovqini (relyef uchun)
float valueNoise2D(float x, float y, uint32_t seed);
float fbm2D(float x, float y, uint32_t seed, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);

} // namespace ert
