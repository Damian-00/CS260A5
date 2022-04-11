#pragma once
#include <chrono>
#include <unordered_map>
#include <string>

namespace engine {
    class window;
    class shader;
    class font;
}
class game
{
  private:
    // Time control
    using clock = std::chrono::high_resolution_clock;
    float             m_game_time{};
    float             m_dt{};
    clock::time_point m_start_time_point;
    clock::time_point m_frame_time_point;

    // Window
    engine::window* m_window;

    // General resources
    engine::shader* m_default_shader;
    engine::font*   m_default_font;

    // AlphaEngine-like states
    void (*m_state_load)();
    void (*m_state_init)(bool, const std::string&, uint16_t, bool);
    void (*m_state_update)();
    void (*m_state_render)();
    void (*m_state_free)();
    void (*m_state_unload)();

    // Input
    std::unordered_map<int, int> m_key_states;
    std::unordered_map<int, int> m_key_states_prev;

  public:
    static game&
    instance()
    {
        static game inst;
        return inst;
    }
    void create(bool is_server, const std::string& address, uint16_t port, bool verbose);
    bool update();
    void destroy();

    void set_state_ingame(bool is_server, const std::string& address, uint16_t port, bool verbose);

    float                      game_time() const { return m_game_time; }
    float                      dt() const { return m_dt; }
    decltype(m_window)         window() const { return m_window; }
    decltype(m_default_shader) shader_default() const { return m_default_shader; }
    decltype(m_default_font)   font_default() const { return m_default_font; }

    bool input_key_pressed(int key) { return m_key_states[key] >= 1; }
    bool input_key_triggered(int key) { return m_key_states[key] >= 1 && m_key_states_prev[key] == 0; }

  private:
    game()                = default;
    game(game const& rhs) = delete;
    game& operator=(game const& rhs) = delete;
};

inline void AE_ASSERT_MESG(...) {}

extern int gAEGameStateNext;
extern int GS_RESULT;