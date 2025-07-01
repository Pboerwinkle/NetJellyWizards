#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <thread>
#include <cstdint>
#include <vector>

int maxConn;
int port;
bool quit = false;
float deltaTime;
const float maxFPS = 100.f;
sf::Time maxFramerate = sf::seconds(1/maxFPS);

std::int8_t gameMode = 0;
std::string mapName = "pillars";
std::int8_t gameState = 0; //0:lobby 1:game 2:endScreen

bool allReady;

//class for players controlled by clients that connect to the server
class Player {
    public:
        int connNum;
        bool disconnected = false;
        std::thread packetListenerThread;
        sf::Packet recvPacket;
        sf::Packet sendPacket;
        std::int16_t pos[2] = {0, 0};
        sf::CircleShape icon;
        std::uint8_t color[3];
        std::string name = "unnamed";
        std::string spells[3] = {"spel1", "spell2", "spelll3"};
        std::int16_t healthMax;
        std::int16_t healthActual;
        std::int16_t effectAmnts[13];
        bool ready;
        std::int8_t team = 0;
        std::int16_t stats[3] = {0, 0, 0};

        //listen for packets from the client
        void packetListener(std::vector<sf::TcpSocket>& Sockets) {
            std::int8_t signalCode;
            std::int8_t stringLength;
            std::uint8_t nextChar;
            std::string recvString;
            while (!quit && Sockets[connNum].receive(recvPacket) != sf::Socket::Disconnected) {
                Sockets[connNum].receive(recvPacket);
                for (int i = 0; i < 9; i++) {
                    recvPacket>>signalCode;
                    if (signalCode == 0) {
                        recvPacket>>color[0]>>color[1]>>color[2];
                    } else if (signalCode == 1) {
                        for (int j = 0; j < 3; j++) {
                            recvPacket>>spells[j];
                        }
                    } else if (signalCode == 2) {
                        recvPacket>>ready;
                    } else if (signalCode == 3) {
                        recvPacket>>healthMax>>healthActual;
                    } else if (signalCode == 4) {
                        for (int j = 0; j < 13; j++) {
                            recvPacket>>effectAmnts[j];
                        }
                    } else if (signalCode == 5) {
                        recvPacket>>pos[0]>>pos[1];
                    } else if (signalCode == 6) {
                        recvPacket>>team;
                    } else if (signalCode == 7) {
                        recvPacket>>stats[0]>>stats[1]>>stats[3];
                    } else if (signalCode == 8) {
                        recvPacket>>name;
                    }
                }
                recvPacket.clear();
            }
            std::cout<<Sockets[connNum].getRemoteAddress()<<" disconnected"<<std::endl;
            Sockets[connNum].disconnect();
            disconnected = true;
        }

        //wait for the packet listener to quit
        void quitListener() {
            packetListenerThread.join();
        }

        //send info to the client
        void sendInfo(std::vector<Player*>& Players, std::vector<int>& activePlayers, std::vector<sf::TcpSocket>& Sockets) {
            std::int16_t playerNum = 0;
            std::int8_t stringLength;
            std::int8_t signalCode;

            //get active players
            for (int i = 0; i < maxConn; i++) {
                if (Players[i] && i != connNum) {
                    activePlayers[playerNum] = i;
                    playerNum += 1;
                }
            }

            //send info
            signalCode = 0;
            sendPacket<<signalCode<<gameMode<<mapName<<playerNum<<gameState;
            for (int i = 0; i < playerNum; i++) {
                signalCode = 1;
                sendPacket<<signalCode;
                signalCode = 0;
                sendPacket<<signalCode<<Players[activePlayers[i]]->color[0]<<Players[activePlayers[i]]->color[1]<<Players[activePlayers[i]]->color[2];
                signalCode = 1;
                sendPacket<<signalCode;
                for (int j = 0; j < 3; j++) {
                    sendPacket<<Players[activePlayers[i]]->spells[j];
                }
                signalCode = 2;
                sendPacket<<signalCode<<Players[activePlayers[i]]->ready;
                signalCode = 3;
                sendPacket<<signalCode<<Players[activePlayers[i]]->healthMax<<Players[activePlayers[i]]->healthActual;
                signalCode = 4;
                sendPacket<<signalCode;
                for (int j = 0; j < 13; j++) {
                    sendPacket<<Players[activePlayers[i]]->effectAmnts[j];
                }
                signalCode = 5;
                sendPacket<<signalCode<<Players[activePlayers[i]]->pos[0]<<Players[activePlayers[i]]->pos[1];
                signalCode = 6;
                sendPacket<<signalCode<<Players[activePlayers[i]]->team;
                signalCode = 7;
                sendPacket<<signalCode<<Players[activePlayers[i]]->stats[0]<<Players[activePlayers[i]]->stats[1]<<Players[activePlayers[i]]->stats[2];
                signalCode = 8;
                sendPacket<<signalCode<<Players[activePlayers[i]]->name;
            }
            signalCode = -1;
            sendPacket<<signalCode;
            Sockets[connNum].send(sendPacket);
            sendPacket.clear();
        }

        //update shape object associated with this player object
        void updateIcon() {
            icon.setPosition(pos[0]-30, pos[1]-30);
        }

        //runs on player creation
        Player (int thisConnNum, std::vector<sf::TcpSocket>& Sockets) {
            connNum = thisConnNum;
            packetListenerThread = std::thread(&Player::packetListener, this, std::ref(Sockets));
            std::fill(std::begin(effectAmnts), std::end(effectAmnts), 0);
            icon = sf::CircleShape(30.f);
            color[0] = 0; color[1] = 255; color[2] = 0;
            icon.setFillColor(sf::Color(color[0], color[1], color[2]));
            ready = false;
        }
};

//listen for new connections
void connListener(std::vector<sf::TcpSocket>& Sockets, std::vector<bool>& openSockets, std::vector<Player*>& Players) {
    int connNum;
    sf::TcpListener listener;
    listener.listen(port);
    listener.setBlocking(false);
    while (!quit) {
        //check for open sockets
        connNum = -1;
        for (int i = 0; i < maxConn; i++) {
            if (openSockets[i]) {
                connNum = i;
                break;
            }
        }

        //if there are no open connections, dont wait for a connection
        if (connNum == -1) {
            continue;
        }

        //accept connection and assign a player object to the connection number
        if (listener.accept(Sockets[connNum]) == sf::Socket::Status::Done) {
            Players[connNum] = new Player(connNum, Sockets);
            openSockets[connNum] = false;
            std::cout<<Sockets[connNum].getRemoteAddress()<<" connected"<<std::endl;
        }
    }
}

int main()
{
    std::ifstream configuration("server.conf");
    configuration>>port>>maxConn;
    configuration.close();

    std::vector<sf::TcpSocket> Sockets(maxConn);
    std::vector<bool> openSockets(maxConn);
    std::vector<int> activePlayers(maxConn);
    std::vector<Player*> Players(maxConn);

    std::fill(std::begin(openSockets), std::end(openSockets), true);
    sf::Clock frameClock;
    sf::Clock packetClock;
    std::thread connListenerThread(connListener, std::ref(Sockets), std::ref(openSockets), std::ref(Players));
    sf::RenderWindow window(sf::VideoMode({600, 400}), "Server");
    sf::Event event;
    while (window.isOpen()) {
        if (frameClock.getElapsedTime() < maxFramerate) {
            sf::sleep(maxFramerate - frameClock.getElapsedTime());
        }
        deltaTime = frameClock.restart().asMicroseconds()/1000000.f;

        //get input
        while (window.pollEvent(event)) {

            //close window and wait for listeners to quit
            if (event.type == sf::Event::Closed) {
                window.close();
                quit = true;
                //wait for sub-listeners to quit
                for (int i = 0; i < maxConn; i++) {
                    if (Players[i]) {
                        Players[i]->quitListener();
                    }
                }

                connListenerThread.join();
                std::cout<<"quitting"<<std::endl;
            }
        }

        if (gameState == 0) {
            for (int i = 0; i < maxConn; i++) {
                if (Players[i]) {
                    if (!Players[i]->ready) {
                        allReady = false;
                        std::cout<<"not ready"<<std::endl;
                        break;
                    } else {
                        allReady = true;
                    }
                }
            }
            if (allReady) {
                gameState = 1;
            }
        }

        //send info to clients
        if (packetClock.getElapsedTime().asMicroseconds()/1000000.f >= 0.01) {
            for (int i = 0; i < maxConn; i++) {
                if (Players[i]) {

                    //check client disconnects and open up sockets
                    if (Players[i]->disconnected) {
                        openSockets[Players[i]->connNum] = true;
                        Players[i] = nullptr;
                        continue;
                    }
                    Players[i]->sendInfo(Players, activePlayers, Sockets);
                }
            }
            packetClock.restart();
        }

        //draw screen
        window.clear();
        for (int i = 0; i < maxConn; i++) {
            if (Players[i]) {
                Players[i]->updateIcon();
                window.draw(Players[i]->icon);
            }
        }
        window.display();
    }
}