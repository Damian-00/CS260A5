#pragma once

namespace engine {
    class texture
    {
      private:
        unsigned m_id{};
        unsigned m_width{};
        unsigned m_height{};

      public:
        ~texture();
        void create(char const* filename);
        void destroy(); 
        void activate(unsigned slot);

        decltype(m_width)  width() const { return m_width; }
        decltype(m_height) height() const { return m_height; }
    };
}