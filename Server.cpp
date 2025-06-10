#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <thread>
#include <cstdint>

const int maxConn = 10;
sf::TcpSocket Sockets[maxConn];
bool openSockets[maxConn];
int port = 50001;
bool quit = false;
std::int16_t* worldInfo;
float deltaTime;
const float maxFPS = 100.f;
sf::Time maxFramerate = sf::seconds(1/maxFPS);

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

        //listen for packets from the client
        void packetListener() {
            while (!quit && Sockets[connNum].receive(recvPacket) != sf::Socket::Disconnected) {
                Sockets[connNum].receive(recvPacket);
                recvPacket>>pos[0]>>pos[1];
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
        void sendInfo(Player* Players[maxConn]) {
            std::int16_t playerNum = 0;
            delete[] worldInfo;
            worldInfo = new std::int16_t[maxConn*2];

            //compile info
            for (int i = 0; i < maxConn; i++) {
                if (Players[i] && i != connNum) {
                    worldInfo[playerNum*2] = Players[i]->pos[0];
                    worldInfo[playerNum*2+1] = Players[i]->pos[1];
                    playerNum += 1;
                }
            }

            //send info
            sendPacket<<playerNum;
            for (int i = 0; i < playerNum*2; i++) {
                sendPacket<<worldInfo[i];
            }
            Sockets[connNum].send(sendPacket);
            sendPacket.clear();
        }

        //update shape object associated with this player object
        void updateIcon() {
            icon.setPosition(pos[0]-30, pos[1]-30);
        }

        //runs on player creation
        Player (int thisConnNum) {
            connNum = thisConnNum;
            packetListenerThread = std::thread(&Player::packetListener, this);
            icon = sf::CircleShape(30.f);
            icon.setFillColor(sf::Color(255, 0, 0));
        }
};
Player* Players[2];

//listen for new connections
void connListener() {
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
            Players[connNum] = new Player(connNum);
            openSockets[connNum] = false;
            std::cout<<Sockets[connNum].getRemoteAddress()<<" connected"<<std::endl;
        }
    }
}

int main()
{
    std::fill(std::begin(openSockets), std::end(openSockets), true);
    sf::Clock frameClock;
    sf::Clock packetClock;
    std::thread connListenerThread(connListener);
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
                    Players[i]->sendInfo(Players);
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