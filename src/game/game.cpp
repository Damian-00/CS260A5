#include "game.hpp"

#include <chrono>
#include <iostream>
#include "engine/opengl.hpp"
#include "engine/window.hpp"
#include "engine/shader.hpp"
#include "engine/font.hpp"

// State ingame
void GameStatePlayLoad(void);
void GameStatePlayInit(bool is_server, const std::string& address, uint16_t port, bool verbose);
void GameStatePlayUpdate(void);
void GameStatePlayDraw(void);
void GameStatePlayFree(void);
void GameStatePlayUnload(void);

// State ingame solo
void GameStatePlayLoadSolo(void);
void GameStatePlayInitSolo(bool is_server, const std::string& address, uint16_t port, bool verbose);
void GameStatePlayUpdateSolo(void);
void GameStatePlayDrawSolo(void);
void GameStatePlayFreeSolo(void);
void GameStatePlayUnloadSolo(void);

namespace {

}

/**
 * @brief 
 * 
 */
void game::create(bool is_server, const std::string& address, uint16_t port, bool verbose, bool is_solo)
{
    // Window
    m_window = new engine::window();
    if (!is_server) {

        m_window->create(1270, 780, "Asteroids Client");
    }
    else {

        m_window->create(1270, 780, "Asteroids Server");

    }

    // Assets
    m_default_shader = engine::shader_default_create();

    m_default_font = new engine::font();
    m_default_font->create("../../../resources/monospaced_24.fnt");

    // General
    m_start_time_point = clock::now();
    m_frame_time_point = m_start_time_point;
    m_game_time        = 0.0f;
	
    set_state_ingame(is_server, address, port, verbose, is_solo);
}

/**
 * @brief 
 * 
 */
bool game::update()
{
    // dt
    auto now           = clock::now();
    auto diff          = now - m_frame_time_point;
    m_frame_time_point = now;
    m_dt               = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() * 0.001f;
    m_game_time        = m_game_time + m_dt;

    // Window update
    should_continue = m_window->update();

    m_key_states_prev = m_key_states;
    for (int i = 0; i < GLFW_KEY_LAST; ++i) {
        m_key_states[i] = glfwGetKey(m_window->handle(), i);
    }

    // States
    m_state_update();
    m_state_render();

    m_window->swap_buffers();
    return should_continue;
}

/**
 * @brief 
 * 
 */
void game::set_state_ingame(bool is_server, const std::string& address, uint16_t port, bool verbose, bool is_solo)
{
    if (m_state_free) m_state_free();
    if (m_state_unload) m_state_unload();

    if (!is_solo)
    {
        m_state_load = &GameStatePlayLoad;
        m_state_init = &GameStatePlayInit;
        m_state_update = &GameStatePlayUpdate;
        m_state_render = &GameStatePlayDraw;
        m_state_free = &GameStatePlayFree;
        m_state_unload = &GameStatePlayUnload;
    }
    else
    {
        m_state_load = &GameStatePlayLoadSolo;
        m_state_init = &GameStatePlayInitSolo;
        m_state_update = &GameStatePlayUpdateSolo;
        m_state_render = &GameStatePlayDrawSolo;
        m_state_free = &GameStatePlayFreeSolo;
        m_state_unload = &GameStatePlayUnloadSolo;		
    }

    m_state_load();
    m_state_init(is_server, address, port, verbose);
}

void game::destroy()
{
    delete m_default_shader;
    m_default_shader = nullptr;
    delete m_default_font;
    m_default_font = nullptr;
    delete m_window;
    m_window = nullptr;
}