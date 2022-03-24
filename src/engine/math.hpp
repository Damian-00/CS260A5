#pragma once

#define GLM_FORCE_INLINE
#define GLM_FORCE_NO_CTOR_INIT
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/integer.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;
using glm::ivec3;

#define PI glm::pi<float>()

inline bool in_range(float v, float min, float max)
{
    return v >= min && v <= max;
}
inline float frand()
{
    return glm::linearRand(0.0f, 1.0f);
}

inline float wrap(float f, float f0, float f1)
{
    
    if (f < f0)
        f += f1 - f0;
    if (f > f1)
        f -= f1 - f0;
    return f;
}

inline float cross_product_mag(vec2 const& v1, vec2 const& v2)
{
    return glm::length(glm::cross(vec3(v1, 0.0f), vec3(v2, 0.0f)));
}

inline bool point_in_aabb(vec2 const& pos, vec2 const& rect, float w, float h)
{
    float w2 = w * 0.5f;
    float h2 = h * 0.5f;
    if (pos.x >= rect.x - w2 && pos.x <= rect.x + w2 &&
        pos.y >= rect.y - h2 && pos.y <= rect.y + h2)
        return true;
    return false;
}

inline bool point_in_sphere(vec2 const& pos, vec2 const& sph, float r)
{
    return glm::length2(pos - sph) <= r * r;
}

inline float aabb_vs_aabb(vec2 const& rect0, float w0, float h0, vec2 const& rect1, float w1, float h1)
{
    float start_x_0 = rect0.x - w0 * 0.5f;
    float end_x_0   = rect0.x + w0 * 0.5f;
    float start_y_0 = rect0.y - h0 * 0.5f;
    float end_y_0   = rect0.y + h0 * 0.5f;
    float start_x_1 = rect1.x - w1 * 0.5f;
    float end_x_1   = rect1.x + w1 * 0.5f;
    float start_y_1 = rect1.y - h1 * 0.5f;
    float end_y_1   = rect1.y + h1 * 0.5f;
    if(start_x_0 > end_x_1) return false;
    if(start_x_1 > end_x_0) return false;
    if(start_y_0 > end_y_1) return false;
    if(start_y_1 > end_y_0) return false;
    return true;
}