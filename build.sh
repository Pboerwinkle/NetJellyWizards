 g++ -c Server.cpp Client.cpp
 g++ Server.o -o Server -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network
 g++ Client.o -o Client -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network