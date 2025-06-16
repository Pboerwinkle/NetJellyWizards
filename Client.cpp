#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdint>

sf::IpAddress serverIp;
int port;
sf::TcpSocket socket;
sf::Packet sendPacket;
sf::Packet recvPacket;
bool quit = false;
float deltaTime;
const float maxFPS = 100.f;
sf::Time maxFramerate = sf::seconds(1/maxFPS);
sf::Mutex globalMutex;

std::int8_t gameMode;
std::int16_t mapCode;
std::int16_t playerNum = 0;
std::int8_t gameState;

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
};

Player thisPlayer;
Player* otherPlayers;
int currentPlayerIndex;

//send info to the server
void sendInfo() {
    std::int8_t signalCode;
    std::int8_t spellLength;
    signalCode = 0;
    sendPacket<<signalCode<<thisPlayer.color[0]<<thisPlayer.color[1]<<thisPlayer.color[2];
    signalCode = 1;
    sendPacket<<signalCode;
    for (int j = 0; j < 3; j++) {
        spellLength = thisPlayer.spells[j].length();
        sendPacket<<spellLength;
        for (int k = 0; k < spellLength; k++) {
            sendPacket<<(std::int8_t)thisPlayer.spells[j][k];
        }
    }
    signalCode = 2;
    sendPacket<<signalCode<<thisPlayer.ready;
    signalCode = 3;
    sendPacket<<signalCode<<thisPlayer.healthMax<<thisPlayer.healthActual;
    signalCode = 4;
    sendPacket<<signalCode;
    for (int j = 0; j < 13; j++) {
        sendPacket<<thisPlayer.effectAmnts[j];
    }
    signalCode = 5;
    sendPacket<<signalCode<<thisPlayer.position[0]<<thisPlayer.position[1];
    signalCode = 6;
    sendPacket<<signalCode<<thisPlayer.team;
    signalCode = 7;
    sendPacket<<signalCode<<thisPlayer.stats[0]<<thisPlayer.stats[1]<<thisPlayer.stats[2];
    socket.send(sendPacket);
    sendPacket.clear();
}

//listen for packets from the server
void serverListener() {
    std::int8_t signalCode;
    std::int8_t spellLength;
    std::int8_t nextChar;
    std::string spell;
    while (!quit) {
        socket.receive(recvPacket);
        signalCode = 0;
        currentPlayerIndex = 0;

        globalMutex.lock();
        delete[] otherPlayers;
        playerNum = 0;
        while (signalCode != -1) {
            recvPacket>>signalCode;
            if (signalCode == 0) {
                recvPacket>>gameMode>>mapCode>>playerNum>>gameState;
                otherPlayers = new Player[playerNum];
            }
            else if (signalCode == 1) {
                for (int i = 0; i < 8; i++) {
                    recvPacket>>signalCode;
                    if (signalCode == 0) {
                        recvPacket>>otherPlayers[currentPlayerIndex].color[0]>>otherPlayers[currentPlayerIndex].color[1]>>otherPlayers[currentPlayerIndex].color[2];
                    } else if (signalCode == 1) {
                        for (int j = 0; j < 3; j++) {
                            recvPacket>>spellLength;
                            spell = "";
                            for (int k = 0; k < spellLength; k++) {
                                recvPacket>>nextChar;
                                spell += char(nextChar);
                            }
                            otherPlayers[currentPlayerIndex].spells[j] = spell;
                        }
                    } else if (signalCode == 2) {
                        recvPacket>>otherPlayers[currentPlayerIndex].ready;
                    } else if (signalCode == 3) {
                        recvPacket>>otherPlayers[currentPlayerIndex].healthMax>>otherPlayers[currentPlayerIndex].healthActual;
                    } else if (signalCode == 4) {
                        for (int j = 0; j < 13; j++) {
                            recvPacket>>otherPlayers[currentPlayerIndex].effectAmnts[j];
                        }
                    } else if (signalCode == 5) {
                        recvPacket>>otherPlayers[currentPlayerIndex].position[0]>>otherPlayers[currentPlayerIndex].position[1];
                    } else if (signalCode == 6) {
                        recvPacket>>otherPlayers[currentPlayerIndex].team;
                    } else if (signalCode == 7) {
                        recvPacket>>otherPlayers[currentPlayerIndex].stats[0]>>otherPlayers[currentPlayerIndex].stats[1]>>otherPlayers[currentPlayerIndex].stats[3];
                    }
                }
                currentPlayerIndex += 1;
            }
        }
        globalMutex.unlock();
        //recvPacket>>playerNum;
        //worldSize = playerNum*2;
        //delete[] worldInfo;
        //worldInfo = new std::int16_t[worldSize];
        //for (int i = 0; i < worldSize; i++) {
        //    recvPacket>>worldInfo[i];
        //}
        
        recvPacket.clear();
    }
    socket.disconnect();
}

const int moveInputBinds[4] = {sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::A, sf::Keyboard::D};
const int moveVectors[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

bool moveInputs[4] = {false, false, false, false};
int rawVel[2] = {0, 0};
float rawDist = 0.f;

float playerPos[2] = {0.f, 0.f};

int moveSpeed = 100;

int main() {
    std::ifstream configuration("client.conf");
    configuration>>serverIp>>port;
    int nextNum;
    configuration>>nextNum; thisPlayer.color[0] = nextNum;
    configuration>>nextNum; thisPlayer.color[1] = nextNum;
    configuration>>nextNum; thisPlayer.color[2] = nextNum;
    configuration>>thisPlayer.spells[0]>>thisPlayer.spells[1]>>thisPlayer.spells[2];
    configuration.close();

    thisPlayer.ready = false;
    thisPlayer.healthMax = 100;
    thisPlayer.healthActual = 100;
    std::fill(std::begin(thisPlayer.effectAmnts), std::end(thisPlayer.effectAmnts), 0);
    thisPlayer.position[0] = 0; thisPlayer.position[1] = 0;
    thisPlayer.team = 0;
    thisPlayer.stats[0] = 0; thisPlayer.stats[1] = 0; thisPlayer.stats[2] = 0;

    socket.connect(serverIp, port);
    std::thread serverListenerThread(&serverListener);

    sf::RenderWindow window(sf::VideoMode({600, 400}), "Client");
    window.setKeyRepeatEnabled(false);
    sf::CircleShape playerIcon(30.f);
    playerIcon.setFillColor(sf::Color(thisPlayer.color[0], thisPlayer.color[1], thisPlayer.color[2]));
    sf::CircleShape otherPlayerIcon(30.f);
    otherPlayerIcon.setFillColor(sf::Color(0, 255, 0));

    sf::Clock frameClock;
    sf::Clock packetClock;
    sf::Event event;

    while (window.isOpen()) {
        if (frameClock.getElapsedTime() < maxFramerate) {
            sf::sleep(maxFramerate - frameClock.getElapsedTime());
        }
        deltaTime = frameClock.restart().asMicroseconds()/1000000.f;

        //get input
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                quit = true;
                serverListenerThread.join();
                window.close();
            }
            //check key presses
            if (event.type == sf::Event::KeyPressed) {
                for (int i = 0; i < 4; i++) {
                    if (event.key.code == moveInputBinds[i]) {
                        moveInputs[i] = true;
                    }
                }
            }
            //check key releases
            if (event.type == sf::Event::KeyReleased) {
                for (int i = 0; i < 4; i++) {
                    if (event.key.code == moveInputBinds[i]) {
                        moveInputs[i] = false;
                    }
                }
            }
        }

        //calculate velocity based on inputs
        std::fill(std::begin(rawVel), std::end(rawVel), 0);
        for (int i = 0; i < 4; i++) {
            if (moveInputs[i]) {
                rawVel[0] += moveVectors[i][0];
                rawVel[1] += moveVectors[i][1];
            }
        }
        rawDist = sqrt(abs(rawVel[0])+abs(rawVel[1]));
        if (rawDist != 0) {
            rawDist = 1/rawDist;
        }

        //update position
        playerPos[0] += rawVel[0]*moveSpeed*rawDist*deltaTime;
        playerPos[1] += rawVel[1]*moveSpeed*rawDist*deltaTime;

        //send info to server
        if (packetClock.getElapsedTime().asMicroseconds()/1000000.f >= 0.01) {
            thisPlayer.position[0] = static_cast<std::int16_t>(playerPos[0]);
            thisPlayer.position[1] = static_cast<std::int16_t>(playerPos[1]);
            sendInfo();
            packetClock.restart();
        }

        //draw screen
        playerIcon.setPosition(playerPos[0]-30, playerPos[1]-30);
        window.clear();
        window.draw(playerIcon);
        globalMutex.lock();
        for (int i = 0; i < playerNum; i++) {
            otherPlayerIcon.setPosition(otherPlayers[i].position[0]-30, otherPlayers[i].position[1]-30);
            otherPlayerIcon.setFillColor(sf::Color(otherPlayers[i].color[0], otherPlayers[i].color[1], otherPlayers[i].color[2]));
            window.draw(otherPlayerIcon);
        }
        globalMutex.unlock();
        window.display();
    }
    delete[] otherPlayers;
}