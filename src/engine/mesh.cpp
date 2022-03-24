#include "mesh.hpp"
#include "opengl.hpp"

namespace engine {
    gfx_triangle::gfx_triangle(float x0, float y0, unsigned col0, float uvx0, float uvy0, float x1, float y1, unsigned col1, float uvx1, float uvy1, float x2, float y2, unsigned col2, float uvx2, float uvy2)
    {
        pos[0] = {x0, y0};
        pos[1] = {x1, y1};
        pos[2] = {x2, y2};

        color[0].w = ((col0 & 0xFF000000) >> 24) / 255.0f; // A
        color[0].x = ((col0 & 0x00FF0000) >> 16) / 255.0f; // R
        color[0].y = ((col0 & 0x0000FF00) >> 8) / 255.0f;  // G
        color[0].z = ((col0 & 0x000000FF) >> 0) / 255.0f;  // B

        color[1].w = ((col1 & 0xFF000000) >> 24) / 255.0f; // A
        color[1].x = ((col1 & 0x00FF0000) >> 16) / 255.0f; // R
        color[1].y = ((col1 & 0x0000FF00) >> 8) / 255.0f;  // G
        color[1].z = ((col1 & 0x000000FF) >> 0) / 255.0f;  // B

        color[2].w = ((col2 & 0xFF000000) >> 24) / 255.0f; // A
        color[2].x = ((col2 & 0x00FF0000) >> 16) / 255.0f; // R
        color[2].y = ((col2 & 0x0000FF00) >> 8) / 255.0f;  // G
        color[2].z = ((col2 & 0x000000FF) >> 0) / 255.0f;  // B

        uv[0] = {uvx0, uvy0};
        uv[1] = {uvx1, uvy1};
        uv[2] = {uvx2, uvy2};
    }

    void mesh::add_triangle(gfx_triangle const& tri)
    {
        m_triangles.push_back(tri);
    }

    void mesh::create()
    {
        glGenBuffers(1, &m_vbo);

        std::vector<float> packed_data;
        packed_data.reserve(m_triangles.size() * 8);
        for (auto const& t : m_triangles) {
            for (int i = 0; i < 3; ++i) {
                packed_data.push_back(t.pos[i].x);
                packed_data.push_back(t.pos[i].y);
                packed_data.push_back(t.color[i].x);
                packed_data.push_back(t.color[i].y);
                packed_data.push_back(t.color[i].z);
                packed_data.push_back(t.color[i].w);
                packed_data.push_back(t.uv[i].x);
                packed_data.push_back(t.uv[i].y);
            }
        }

        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * packed_data.size(), packed_data.data(), GL_STATIC_DRAW);

        // VAO
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);

        // Colors
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 2));

        // UVs
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));

        glBindVertexArray(0);
    }

    void mesh::destroy()
    {
        if (m_vao) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        if (m_vbo) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
    }

    void mesh::draw()
    {
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, m_triangles.size() * 3);
    }

    void mesh::upload_dynamic_data(std::vector<float> const& packed_data, unsigned vertex_count)
    {
        m_triangles.resize(vertex_count / 3);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * packed_data.size(), packed_data.data(), GL_DYNAMIC_DRAW);
    }
}