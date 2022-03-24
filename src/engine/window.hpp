#pragma once

#include "math.hpp"

struct GLFWwindow;

namespace engine {
    class window
    {
      private:
        GLFWwindow* m_window = {};
        glm::ivec2  m_size   = {};

      public:
        ~window() { destroy(); };
        bool create(int w, int h, const char* window_name);
        bool update();
        void swap_buffers();
        void destroy();

        [[nodiscard]] glm::ivec2  size() const noexcept { return m_size; }
        [[nodiscard]] GLFWwindow* handle() const noexcept { return m_window; }
    };
}