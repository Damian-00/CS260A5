#include "font.hpp"

#include "texture.hpp"
#include "mesh.hpp"
#include "math.hpp"
#include "shader.hpp"
#include "opengl.hpp"

#include <fstream>
#include <cassert>
#include <sstream>
#include <cstring>

namespace engine {

    /**
     * @brief Destroy the font::font object
     * 
     */
    font::~font()
    {
        destroy();
    }

    /**
     * @brief 
     * 
     * @param filename 
     */
    void font::create(char const* filename)
    {
        assert(!m_texture);

        //
        std::fstream f(filename, std::ios::in);
        if (!f.is_open()) {
            std::stringstream ss;
            ss << "Could not open font file: " << filename;
            throw std::runtime_error(ss.str());
        }

        while (!f.bad() && !f.eof()) {
            std::string token;
            f >> token;
            if (token.empty()) {
            } else if (token == "info") {
            } else if (token.starts_with("face=")) {
            } else if (token.starts_with("size=")) {
            } else if (token.starts_with("bold=")) {
            } else if (token.starts_with("italic=")) {
            } else if (token.starts_with("charset=")) {
            } else if (token.starts_with("unicode=")) {
            } else if (token.starts_with("stretchH=")) {
            } else if (token.starts_with("smooth=")) {
            } else if (token.starts_with("aa=")) {
            } else if (token.starts_with("padding=")) {
            } else if (token.starts_with("spacing=")) {
            } else if (token.starts_with("common")) {
            } else if (token.starts_with("lineHeight=")) {
                m_line_height = atoi(token.c_str() + std::strlen("lineHeight="));
            } else if (token.starts_with("base=")) {
                m_base = atoi(token.c_str() + std::strlen("base="));
            } else if (token.starts_with("scaleW=")) {
            } else if (token.starts_with("scaleH=")) {
            } else if (token.starts_with("pages=")) {
            } else if (token.starts_with("packed=")) {
            } else if (token.starts_with("page")) {
                parse_page(f);
            } else {
                std::stringstream ss;
                ss << "Token '" << token << "' not recognized";
                throw std::runtime_error(ss.str());
            }
        }

        // Create the dynamic mesh
        m_dynamic_mesh = new mesh;
        m_dynamic_mesh->create();

        m_font_shader = shader_font_create();
    }

    /**
     * @brief 
     * 
     */
    void font::destroy()
    {
        delete m_texture;
        m_texture = nullptr;
        delete m_dynamic_mesh;
        m_dynamic_mesh = nullptr;
        delete m_font_shader;
        m_font_shader = nullptr;
    }

    /**
     * @brief 
     * 
     * @param text 
     * @param x 
     * @param y 
     * @param size 
     */
    void font::render(char const* text, int x, int y, int size, mat4 const& vp)
    {
        // The glyph quad
        constexpr std::array<vec3, 6> const c_quad_pos = {{
            {-0.5f, -0.5f, 0.0f},
            {0.5f, -0.5f, 0.0f},
            {0.5f, 0.5f, 0.0f},
            {-0.5f, 0.5f, 0.0f},
            {-0.5f, -0.5f, 0.0f},
            {0.5f, 0.5f, 0.0f},
        }};
        constexpr std::array<vec2, 6> const c_quad_uv  = {{
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f},
            {1.0f, 1.0f},
        }};

        ivec2 currentPosition = {};

        std::vector<float> packed_data;
        packed_data.reserve(std::strlen(text) * 6 * 8);

        // Move through the text
        while (*text) {
            // Find character info
            auto const c  = (uint32_t)*text;
            auto       it = m_chars.find(c);
            if (it == m_chars.end()) {
                it = m_chars.find((uint32_t)'?');
                assert(it != m_chars.end());
            }
            auto const& glyphData = it->second;

            // Compute render data
            vec2 uvScale    = {glyphData.width, glyphData.height};
            vec2 uvOffset   = {glyphData.x, glyphData.y};
            vec2 quadScale  = {glyphData.width, glyphData.height};
            vec2 quadOffset = {currentPosition.x + glyphData.xoffset, currentPosition.y - (glyphData.yoffset + quadScale.y) - (m_line_height - m_base)};
            currentPosition.x += glyphData.xadvance;

            // Adjust UV
            ivec2 textureSize = {m_texture->width(), m_texture->height()};
            uvScale.x         = uvScale.x / textureSize.x;
            uvScale.y         = uvScale.y / textureSize.y;
            uvOffset.x        = uvOffset.x / textureSize.x;
            uvOffset.y        = 1.0f - uvOffset.y / textureSize.y - uvScale.y;

            // Upload data
            auto m2w       = glm::translate(vec3(quadOffset + vec2(x,y), 0)) * glm::scale(vec3(quadScale, 1)* float(size)/float(m_line_height));
            auto uv_matrix = glm::translate(vec3(uvOffset, 0)) * glm::scale(vec3(uvScale, 1));

            for (int i = 0; i < 6; ++i) {
                auto const& quad_pos  = c_quad_pos[i];
                auto const& quad_uv   = c_quad_uv[i];
                auto        final_pos = m2w * vec4((quad_pos + vec3(0.5f, 0.5f, 0.0f)), 1.0f);
                auto        final_uv  = uv_matrix * vec4(quad_uv, 0.0f, 1.0f);

                packed_data.push_back(final_pos.x);
                packed_data.push_back(final_pos.y);
                packed_data.push_back(1.0f);
                packed_data.push_back(1.0f);
                packed_data.push_back(1.0f);
                packed_data.push_back(1.0f);
                packed_data.push_back(final_uv.x);
                packed_data.push_back(final_uv.y);
            }
            text++;
        }

        m_texture->activate(0);

        //
        m_dynamic_mesh->upload_dynamic_data(packed_data, packed_data.size() / 8);

        // Actual render        
        m_font_shader->use();
        m_font_shader->set_uniform(0, vp);
        m_font_shader->set_uniform(1, 0);

        // State
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);         
        m_dynamic_mesh->draw();
        glDisable(GL_BLEND);
    }

    /**
     * @brief 
     * 
     * @param is 
     */
    void font::parse_page(std::istream& is)
    {
        std::string token;
        is >> token;
        assert(token.starts_with("id="));
        is >> token;
        assert(token.starts_with("file="));
        auto filename = token.substr(std::strlen("file="));
        if (filename.starts_with("\"")) filename = filename.substr(1, filename.size() - 2); // Remove quotes
        // Create texture
        assert(!m_texture);
        m_texture = new engine::texture;
        m_texture->create(filename.c_str());
        is >> token;
        assert(token.starts_with("chars"));
        is >> token;
        assert(token.starts_with("count="));
        int char_count = atoi(token.c_str() + std::strlen("count="));
        for (int i = 0; i < char_count; ++i) {
            auto ch_info        = parse_char(is);
            m_chars[ch_info.id] = ch_info;
        }
    }

    /**
     * @brief 
     * 
     * @param is 
     */
    font::char_info font::parse_char(std::istream& is)
    {
        char_info info{};

        std::string token;
        is >> token;
        assert(token.starts_with("char"));

        is >> token;
        assert(token.starts_with("id="));
        info.id = atoi(token.c_str() + std::strlen("id="));

        is >> token;
        assert(token.starts_with("x="));
        info.x = atoi(token.c_str() + std::strlen("x="));

        is >> token;
        assert(token.starts_with("y="));
        info.y = atoi(token.c_str() + std::strlen("y="));

        is >> token;
        assert(token.starts_with("width="));
        info.width = atoi(token.c_str() + std::strlen("width="));

        is >> token;
        assert(token.starts_with("height="));
        info.height = atoi(token.c_str() + std::strlen("height="));

        is >> token;
        assert(token.starts_with("xoffset="));
        info.xoffset = atoi(token.c_str() + std::strlen("xoffset="));

        is >> token;
        assert(token.starts_with("yoffset="));
        info.yoffset = atoi(token.c_str() + std::strlen("yoffset="));

        is >> token;
        assert(token.starts_with("xadvance="));
        info.xadvance = atoi(token.c_str() + std::strlen("xadvance="));

        is >> token;
        assert(token.starts_with("page="));

        is >> token;
        assert(token.starts_with("chnl="));
        return info;
    }
}