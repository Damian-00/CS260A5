#pragma once
#include "math.hpp"
#include <array>
#include <vector>

namespace engine {
    struct gfx_triangle
    {
        gfx_triangle() = default;
        gfx_triangle(float x0, float y0, unsigned col0, float uvx0, float uvy0, float x1, float y1, unsigned col1, float uvx1, float uvy1, float x2, float y2, unsigned col2, float uvx2, float uvy2);
        std::array<glm::vec2, 3> pos;
        std::array<glm::vec4, 3> color;
        std::array<glm::vec2, 3> uv;
    };
    class mesh
    {
      private:
        unsigned                  m_vbo;
        unsigned                  m_vao;
        std::vector<gfx_triangle> m_triangles;

      public:
        void add_triangle(gfx_triangle const& tri);
        void create();
        void destroy();
        void draw();
        void upload_dynamic_data(std::vector<float> const& packed_data, unsigned vertex_count);
    };
}