#include "engine/window.hpp"
#include "engine/opengl.hpp"
#include "game/game.hpp"

int main()
{
    game::instance().create();
    bool exit = false;
    do {
        exit = !game::instance().update();
    } while (!exit);
    game::instance().destroy();
    return 0;
}
