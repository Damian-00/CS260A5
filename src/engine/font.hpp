#pragma once

#include "math.hpp"
#include <istream>
#include <unordered_map>

namespace engine {
    class texture;
    class mesh;
    class shader;
    class font
    {
      private:
        struct char_info
        {
            int id;
            int x;
            int y;
            int width;
            int height;
            int xoffset;
            int yoffset;
            int xadvance;
        };

        texture*                           m_texture{};
        shader*                            m_font_shader;
        mesh*                              m_dynamic_mesh;
        uint32_t                           m_line_height;
        uint32_t                           m_base;
        std::unordered_map<int, char_info> m_chars;

      public:
        ~font();
        void create(char const* filename);
        void destroy();
        void render(char const* text, int x, int y, int size, mat4 const& vp, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

      protected:
        void      parse_page(std::istream& is);
        char_info parse_char(std::istream& is);
    };
}