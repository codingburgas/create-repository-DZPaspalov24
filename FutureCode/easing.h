#pragma once
#include <cmath>

// Header-only easing functions. All take t in [0,1] and return [0,1] (mostly).
// Used by every animated UI value in the app.

namespace Easing {

inline float Linear(float t) { return t; }

inline float InQuad(float t)    { return t * t; }
inline float OutQuad(float t)   { return 1.0f - (1.0f - t) * (1.0f - t); }
inline float InOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

inline float InCubic(float t)    { return t * t * t; }
inline float OutCubic(float t)   { float u = 1.0f - t; return 1.0f - u * u * u; }
inline float InOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

inline float OutQuart(float t) { float u = 1.0f - t; return 1.0f - u * u * u * u; }
inline float InOutQuart(float t) {
    return t < 0.5f ? 8.0f * t * t * t * t
                    : 1.0f - powf(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

inline float OutExpo(float t) {
    return t >= 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}

inline float OutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

inline float OutElastic(float t) {
    const float c4 = (2.0f * 3.14159265f) / 3.0f;
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

// Smoothly approaches `target` from `current` over time. Frame-rate independent.
// `speed` is roughly "how many seconds to converge" (higher = slower).
inline float Approach(float current, float target, float speed, float dt) {
    if (speed <= 0.0001f) return target;
    float k = 1.0f - expf(-dt / speed);
    return current + (target - current) * k;
}

inline float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace Easing
