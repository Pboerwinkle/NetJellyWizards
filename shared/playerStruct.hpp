#ifndef PLAYERSTRUCT_HPP
#define PLAYERSTRUCT_HPP
#include <cstdint>

struct Player {
    std::uint8_t color[3]; //r, g, b
    std::string spells[3]; //primary, secondary, tertiary
    bool ready;
    std::int16_t healthMax;
    std::int16_t healthActual;
    std::int16_t effectAmnts[13];
    std::int16_t position[2]; //x, y
    std::int8_t team;
    std::int16_t stats[3]; //kills, deaths, goal
    std::string name;
};

#endif
