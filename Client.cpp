#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <cmath>
#include <cstdint>

sf::IpAddress serverIp;
int port = 50001;
sf::TcpSocket socket;
sf::Packet sendPacket;
sf::Packet recvPacket;
bool quit = false;
float deltaTime;
std::int16_t* worldInfo;
int worldSize;
std::int16_t playerNum;
const float maxFPS = 100.f;
sf::Time maxFramerate = sf::seconds(1/maxFPS);

//listen for packets from the server
void serverListener() {
    sf::Clock listenClock;
    while (!quit) {
        if (listenClock.getElapsedTime() < maxFramerate) {
            sf::sleep(maxFramerate - listenClock.getElapsedTime());
        }
        listenClock.restart();
        playerNum = 0;
        socket.receive(recvPacket);
        recvPacket>>playerNum;
        worldSize = playerNum*2;
        delete[] worldInfo;
        worldInfo = new std::int16_t[worldSize];
        for (int i = 0; i < worldSize; i++) {
            recvPacket>>worldInfo[i];
        }
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
    sf::Clock frameClock;
    sf::Clock packetClock;
    std::cout << "Server address: ";
    std::cin >> serverIp;
    socket.connect(serverIp, port);
    std::thread serverListenerThread(&serverListener);

    sf::RenderWindow window(sf::VideoMode({600, 400}), "Client");
    window.setKeyRepeatEnabled(false);
    sf::CircleShape player(30.f);
    player.setFillColor(sf::Color(255, 0, 0));
    sf::CircleShape otherPlayer(30.f);
    otherPlayer.setFillColor(sf::Color(0, 255, 0));

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

        //send position to server
        if (packetClock.getElapsedTime().asMicroseconds()/1000000.f >= 0.01) {
            sendPacket<<static_cast<std::int16_t>(playerPos[0])<<static_cast<std::int16_t>(playerPos[1]);
            socket.send(sendPacket);
            sendPacket.clear();
            packetClock.restart();
        }

        //draw screen
        player.setPosition(playerPos[0]-30, playerPos[1]-30);
        window.clear();
        window.draw(player);
        for (int i = 0; i < worldSize/2; i++) {
            otherPlayer.setPosition(worldInfo[i*2]-30, worldInfo[i*2+1]-30);
            window.draw(otherPlayer);
        }
        window.display();
    }
}