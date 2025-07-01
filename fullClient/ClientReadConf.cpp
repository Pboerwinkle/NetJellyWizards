#include <iostream>
#include <fstream>
#include <cstdint>
#include <cmath>
#include "../shared/playerStruct.hpp"

void readConf(sf::IpAddress& serverIp, int& port, std::uint16_t (&windowSize)[2], std::uint16_t& windowScale, Player& thisPlayer) {
    std::ifstream configuration("client.conf");
    configuration>>serverIp>>port;
    configuration>>windowSize[0]>>windowSize[1]>>windowScale;
    configuration>>thisPlayer.name;
    int nextNum;
    configuration>>nextNum; thisPlayer.color[0] = nextNum;
    configuration>>nextNum; thisPlayer.color[1] = nextNum;
    configuration>>nextNum; thisPlayer.color[2] = nextNum;
    configuration>>thisPlayer.spells[0]>>thisPlayer.spells[1]>>thisPlayer.spells[2];
    configuration.close();
}
