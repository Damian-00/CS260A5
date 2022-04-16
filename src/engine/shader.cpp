#include "shader.hpp"
#include "opengl.hpp"
#include <string>
#include <cstring>
#include <cassert>
#include <iostream>
#include <array>

namespace {
    constexpr char const* c_vertex_shader = R"(
        #version 440 core

        layout(location = 0) in vec2 attr_position;        
        layout(location = 1) in vec4 attr_color;

        layout(location = 0) uniform mat4 uniform_mvp;
        layout(location = 1) uniform vec4 mod_color;

        out vec4 var_color;

        void main()
        {
            vec4 vertex = vec4(attr_position, 0.0f, 1.0f);
            var_color   = attr_color * mod_color;
            gl_Position = uniform_mvp * vertex;
        }
    )";

    constexpr char const* c_fragment_shader = R"(
        #version 440 core

        in vec4  var_color;
        out vec4 out_color;

        void main()
        {
            out_color = var_color;
        }
    )";

    constexpr char const* c_font_vertex_shader = R"(
        #version 440 core

        layout(location = 0) in vec2 attr_position;        
        layout(location = 1) in vec4 attr_color;
        layout(location = 2) in vec2 attr_uv;

        layout(location = 0) uniform mat4 uniform_mvp;

        out vec4 var_color;
        out vec2 var_uv;

        void main()
        {
            vec4 vertex = vec4(attr_position, 0.0f, 1.0f);
            var_color   = attr_color;
            var_uv      = attr_uv;
            gl_Position = uniform_mvp * vertex;
        }
    )";

    constexpr char const* c_font_fragment_shader = R"(
        #version 440 core

        layout(location=1) uniform sampler2D uniform_texture;

        in vec2  var_uv;
        in vec4  var_color;
        out vec4 out_color;

        void main()
        {
            vec4 texture_color = texture(uniform_texture, var_uv);
	        out_color          = texture_color * var_color;
        }
    )";

    std::ostream& shader_dump_info(std::ostream& os, unsigned shader_object)
    {
        // Check if compiled
        GLint compiled = 0;
        glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);
        os << "Compile status: " << (compiled ? "OK" : "ERROR") << "\n";

        // Retrieve information
        GLint info_len = 0;
        glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 0) {
            std::string info_log(info_len, '\0');
            glGetShaderInfoLog(shader_object, info_len, &info_len, &info_log[0]);
            os << info_log;
        }
        return os;
    }

    std::ostream& shader_program_dump_info(std::ostream& os, unsigned shader_program)
    {
        assert(shader_program);

        // Retrieve attached shaders
        std::array<GLuint, 8> shaders{};
        GLsizei               count = 0;
        glGetAttachedShaders(shader_program, shaders.size(), &count, shaders.data());

        // Check if linked
        GLint linked = 0;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &linked);
        os << "Link status: " << (linked ? "OK" : "ERROR") << "\n";
        os << "Shader count: " << count << "\n";

        // Retrieve log
        GLsizei info_len = 0;
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 0) {
            std::string info_log(info_len, '\0');
            glGetProgramInfoLog(shader_program, info_len, &info_len, &info_log[0]);
            os << info_log;
        }

        // Dump information for each shader
        for (GLsizei i = 0; i < count; ++i) {
            shader_dump_info(os, shaders[i]);
        }

        return os;
    }

    unsigned shader_compile(unsigned type, char const* src)
    {
        unsigned shader_object = 0;
        shader_object          = glCreateShader(type);
        assert(shader_object);

        // Upload source
        const char* blocks[]     = {src};
        const int   blockSizes[] = {static_cast<int>(std::strlen(src))};
        glShaderSource(shader_object, 1, blocks, blockSizes);

        // Compile it
        glCompileShader(shader_object);

        // Retrieve result
        std::string info_log;
        GLint       info_len = 0;
        glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 0) {
            info_log.resize(static_cast<unsigned>(info_len), '\0');
            GLsizei written = 0;
            glGetShaderInfoLog(shader_object, info_len, &written, &info_log[0]);
        }

        // Check if compiled
        GLint compiled = 0;
        glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);

        if (!compiled) {
            shader_dump_info(std::cerr, shader_object);
            throw std::runtime_error(info_log);
        }
        return shader_object;
    }

}
namespace engine {
    void shader::create(char const* frag, char const* vert)
    {
        m_program = glCreateProgram();

        m_vertex = shader_compile(GL_VERTEX_SHADER, vert);
        if (!m_vertex) {
            throw std::runtime_error("Could not compile vertex shader");
        }
        shader_dump_info(std::cout, m_vertex);

        assert(!m_fragment);
        m_fragment = shader_compile(GL_FRAGMENT_SHADER, frag);
        if (!m_fragment) {
            throw std::runtime_error("Could not compile fragment shader");
        }
        shader_dump_info(std::cout, m_fragment);

        // Attach modules
        glAttachShader(m_program, m_vertex);
        glAttachShader(m_program, m_fragment);

        // Linkage
        glLinkProgram(m_program);
        shader_program_dump_info(std::cout, m_program);
    }

    void shader::use()
    {
        glUseProgram(m_program);
    }
    void shader::destroy()
    {
        // Destroy program
        if (m_program) {
            glDeleteProgram(m_program);
            m_program = 0;
        }

        // Destroy modules
        if (m_fragment) {
            glDeleteShader(m_fragment);
            m_fragment = 0;
        }
        if (m_vertex) {
            glDeleteShader(m_vertex);
            m_vertex = 0;
        }
    }

    void shader::set_uniform(unsigned loc, mat4 const& value)
    {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
    }

    void shader::set_uniform(unsigned loc, int value)
    {
        glUniform1i(loc, value);
    }

    void shader::set_uniform(unsigned loc, vec4 value)
    {
        glUniform4f(loc, value.x, value.y, value.z, value.w);
    }

    shader* shader_default_create()
    {
        shader* sh = new shader();
        sh->create(c_fragment_shader, c_vertex_shader);
        return sh;
    }

    shader* shader_font_create()
    {
        shader* sh = new shader();
        sh->create(c_font_fragment_shader, c_font_vertex_shader);
        return sh;
    }
}