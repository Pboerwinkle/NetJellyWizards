#include <SFML/Graphics.hpp>
#include <cstdint>
#include <cmath>
#include <string>
#include "../shared/playerStruct.hpp"

std::string writePlayerDescription(Player& player) {
    std::string playerDescription = "Name: "+player.name;
    for (int i = 0; i < 3; i++) {
        playerDescription += "\nSpell "+std::to_string(i)+": "+player.spells[i];
    }
    playerDescription += "\nReady: "+std::to_string(1*player.ready);
    return playerDescription;
}

void lobbyInterface(sf::RenderWindow& window, Player& thisPlayer, std::int16_t playerNum, Player otherPlayers[], std::uint16_t windowSize[2], bool& quit, sf::Text (&lobbyText)[4], int UIWidth1, int UIWidth2, int& playerSelected, int gameMode, std::string gameModeLookup[1], std::string mapName, sf::Sprite readyButton) {
    sf::Event event;
    float playerRadius = windowSize[1]/20.f;
    sf::CircleShape playerIcon(playerRadius);
    sf::RectangleShape playerSelectionIndicator({playerRadius*2, playerRadius*2});
    sf::RectangleShape playerInfoHighlight({static_cast<float>(windowSize[0]-UIWidth1), UIWidth2-playerRadius*2});
    sf::RectangleShape roundInfoBorder({1, static_cast<float>(windowSize[1])});
    playerInfoHighlight.setPosition(UIWidth1, playerRadius*2);
    playerSelectionIndicator.setFillColor(sf::Color(0, 0, 0));
    playerInfoHighlight.setFillColor(sf::Color(0, 0, 0));
    roundInfoBorder.setPosition(UIWidth1, 0);
    roundInfoBorder.setFillColor(sf::Color(0, 0, 0));

    readyButton.setPosition(UIWidth1+windowSize[0]/50.f, windowSize[1]-(50*readyButton.getScale().y));

    //get input
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            quit = true;
        } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == 0) {
            if (event.mouseButton.x*windowSize[0]/window.getSize().x >= UIWidth1 && event.mouseButton.y*windowSize[1]/window.getSize().y <= playerRadius*2) {
                playerSelected = (event.mouseButton.x*windowSize[0]/window.getSize().x-UIWidth1)/(playerRadius*2);
            } else if (event.mouseButton.y*windowSize[1]/window.getSize().y >= windowSize[1]-(50*readyButton.getScale().y)) {
                if (event.mouseButton.x*windowSize[0]/window.getSize().x >= UIWidth1+windowSize[0]/50.f && event.mouseButton.x*windowSize[0]/window.getSize().x < UIWidth1+windowSize[0]/50.f+(50*readyButton.getScale().x)) {
                    thisPlayer.ready = !thisPlayer.ready;
                }
            }
        }
    }
    if (playerSelected >= playerNum || playerSelected == -1) {
        playerSelected = playerNum-1;
    }
    if (playerSelected != -1) {
        playerSelectionIndicator.setPosition(UIWidth1+playerSelected*playerRadius*2, 0);
    } else {
        playerSelectionIndicator.setPosition(-playerRadius*2, 0);
    }

    lobbyText[0].setPosition({0, 0});
    lobbyText[0].setString("Game Mode: "+gameModeLookup[gameMode]+"\nMap Name: "+mapName);
    lobbyText[1].setPosition({UIWidth1+windowSize[0]/50.f, playerRadius*2});
    if (playerSelected != -1) {
        lobbyText[1].setString(writePlayerDescription(otherPlayers[playerSelected]));
    } else {
        lobbyText[1].setString("");
    }
    lobbyText[2].setPosition({UIWidth1+windowSize[0]/50.f, UIWidth2+windowSize[0]/50.f});
    lobbyText[2].setString(writePlayerDescription(thisPlayer));
    lobbyText[3].setPosition({0, 0});

    window.clear(sf::Color(40, 40, 40));
    window.draw(playerSelectionIndicator);
    window.draw(playerInfoHighlight);
    window.draw(roundInfoBorder);

    for (int i = 0; i < playerNum; i++) {
        playerIcon.setPosition(UIWidth1+i*(playerRadius*2), 0);
        playerIcon.setFillColor(sf::Color(otherPlayers[i].color[0], otherPlayers[i].color[1], otherPlayers[i].color[2]));
        window.draw(playerIcon);
    }
    playerIcon.setPosition(windowSize[0]-playerRadius*2, UIWidth2);
    playerIcon.setFillColor(sf::Color(thisPlayer.color[0], thisPlayer.color[1], thisPlayer.color[2]));
    window.draw(playerIcon);

    for (int i = 0; i < 4; i++) {
        window.draw(lobbyText[i]);
    }

    window.draw(readyButton);
    window.display();
}

void gameInterface(sf::RenderWindow& window, Player& thisPlayer, std::int16_t playerNum, Player otherPlayers[], float (&playerPos)[2], bool (&moveInputs)[4], int moveInputBinds[4], int moveVectors[4][2], int moveSpeed, bool& quit, float deltaTime) {
    sf::Event event;
    int rawVel[2] = {0, 0};
    float rawDist = 0.f;
    sf::CircleShape playerIcon(30.f);
    playerIcon.setFillColor(sf::Color(thisPlayer.color[0], thisPlayer.color[1], thisPlayer.color[2]));

    //get input
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            quit = true;
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

    //draw screen
    playerIcon.setPosition(playerPos[0]-30, playerPos[1]-30);
    window.clear();
    window.draw(playerIcon);

    for (int i = 0; i < playerNum; i++) {
        playerIcon.setPosition(otherPlayers[i].position[0]-30, otherPlayers[i].position[1]-30);
        playerIcon.setFillColor(sf::Color(otherPlayers[i].color[0], otherPlayers[i].color[1], otherPlayers[i].color[2]));
        window.draw(playerIcon);
    }

    window.display();
}

void endInterface(sf::RenderWindow& window, Player& thisPlayer, std::int16_t playerNum, Player otherPlayers[]) {
}