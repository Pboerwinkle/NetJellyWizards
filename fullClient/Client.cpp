#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdint>
#include "../shared/playerStruct.hpp"
#include "ClientInterface.cpp"
#include "ClientReadConf.cpp"

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
std::string mapName;
std::int16_t playerNum = 0;
std::int8_t gameState;

Player thisPlayer;
Player* otherPlayers;
int currentPlayerIndex;

//send info to the server
void sendInfo() {
    std::int8_t signalCode;
    std::int8_t stringLength;
    signalCode = 0;
    sendPacket<<signalCode<<thisPlayer.color[0]<<thisPlayer.color[1]<<thisPlayer.color[2];
    signalCode = 1;
    sendPacket<<signalCode;
    for (int j = 0; j < 3; j++) {
        sendPacket<<thisPlayer.spells[j];
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
    signalCode = 8;
    sendPacket<<signalCode<<thisPlayer.name;

    socket.send(sendPacket);
    sendPacket.clear();
}

//listen for packets from the server
void serverListener() {
    std::int8_t signalCode;
    std::int8_t stringLength;
    std::uint8_t nextChar;
    std::string recvString;
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
                recvPacket>>gameMode>>mapName>>playerNum>>gameState;
                otherPlayers = new Player[playerNum];
            }
            else if (signalCode == 1) {
                for (int i = 0; i < 9; i++) {
                    recvPacket>>signalCode;
                    if (signalCode == 0) {
                        recvPacket>>otherPlayers[currentPlayerIndex].color[0]>>otherPlayers[currentPlayerIndex].color[1]>>otherPlayers[currentPlayerIndex].color[2];
                    } else if (signalCode == 1) {
                        for (int j = 0; j < 3; j++) {
                            recvPacket>>otherPlayers[currentPlayerIndex].spells[j];
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
                    } else if (signalCode == 8) {
                        recvPacket>>otherPlayers[currentPlayerIndex].name;
                    }
                }
                currentPlayerIndex += 1;
            }
        }
        globalMutex.unlock();
        
        recvPacket.clear();
    }
    socket.disconnect();
}

// gamemode: 0
sf::Texture buttonsTexture;
sf::Sprite readyButton;
float defaultButtonScale[2] = {1/600.f, 1/400.f};
sf::Font lobbyFont;
sf::Text lobbyText[4];
int UIWidth1;
int UIWidth2;
int playerSelected = 0;
std::string gameModeLookup[1] = {"Capture the Flag"};

// gamemode: 1
int moveInputBinds[4] = {sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::A, sf::Keyboard::D};
int moveVectors[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

bool moveInputs[4] = {false, false, false, false};

float playerPos[2] = {0.f, 0.f};

int moveSpeed = 100;

int main() {
    thisPlayer.ready = false;
    thisPlayer.healthMax = 100;
    thisPlayer.healthActual = 100;
    std::fill(std::begin(thisPlayer.effectAmnts), std::end(thisPlayer.effectAmnts), 0);
    thisPlayer.position[0] = 0; thisPlayer.position[1] = 0;
    thisPlayer.team = 0;
    thisPlayer.stats[0] = 0; thisPlayer.stats[1] = 0; thisPlayer.stats[2] = 0;

    std::uint16_t windowSize[2] = {0, 0};
    std::uint16_t windowScale;
    readConf(serverIp, port, windowSize, windowScale, thisPlayer);

    socket.connect(serverIp, port);
    std::thread serverListenerThread(&serverListener);

    sf::RenderWindow window(sf::VideoMode(windowSize[0], windowSize[1]), "Client", sf::Style::Close);
    window.setSize({static_cast<std::uint16_t>(windowSize[0]*windowScale/100), static_cast<std::uint16_t>(windowSize[1]*windowScale/100)});
    window.setKeyRepeatEnabled(false);
    sf::CircleShape otherPlayerIcon(30.f);
    otherPlayerIcon.setFillColor(sf::Color(0, 255, 0));

    buttonsTexture.loadFromFile("graphicalAssets/buttons.png");
    readyButton.setTexture(buttonsTexture);
    readyButton.setTextureRect(sf::IntRect(0, 0, 50, 50));
    readyButton.setScale(defaultButtonScale[0]*windowSize[0], defaultButtonScale[0]*windowSize[0]);

    lobbyFont.loadFromFile("graphicalAssets/arial.ttf");
    for (int i = 0; i < 4; i++) {
        lobbyText[i].setFont(lobbyFont);
        lobbyText[i].setCharacterSize(windowSize[0]/50);
        lobbyText[i].setStyle(sf::Text::Regular);
    }
    UIWidth1 = windowSize[0]/3;
    UIWidth2 = windowSize[1]/3;

    sf::Clock frameClock;
    sf::Clock packetClock;

    while (window.isOpen()) {
        if (frameClock.getElapsedTime() < maxFramerate) {
            sf::sleep(maxFramerate - frameClock.getElapsedTime());
        }
        deltaTime = frameClock.restart().asMicroseconds()/1000000.f;

        globalMutex.lock();
        if (gameState == 0) {
            lobbyInterface(window, thisPlayer, playerNum, otherPlayers, windowSize, quit, lobbyText, UIWidth1, UIWidth2, playerSelected, gameMode, gameModeLookup, mapName, readyButton);
        } else if (gameState == 1) {
            gameInterface(window, thisPlayer, playerNum, otherPlayers, playerPos, moveInputs, moveInputBinds, moveVectors, moveSpeed, quit, deltaTime);
        } else if (gameState == 2) {
        }
        globalMutex.unlock();

        if (quit) {
            serverListenerThread.join();
            window.close();
        }

        //send info to server
        if (packetClock.getElapsedTime().asMicroseconds()/1000000.f >= 0.01) {
            thisPlayer.position[0] = static_cast<std::int16_t>(playerPos[0]);
            thisPlayer.position[1] = static_cast<std::int16_t>(playerPos[1]);
            sendInfo();
            packetClock.restart();
        }
    }
    delete[] otherPlayers;
}