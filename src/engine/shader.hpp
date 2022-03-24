#pragma once
#include "math.hpp"
namespace engine {
    class shader
    {
      private:
        unsigned m_program;
        unsigned m_vertex;
        unsigned m_fragment;

      public:
        void create(char const* frag, char const* vert);
        void use();
        void destroy();
        void set_uniform(unsigned loc, mat4 const& value);
        void set_uniform(unsigned loc, int value);
    };

    shader* shader_default_create();
    shader* shader_font_create();
}