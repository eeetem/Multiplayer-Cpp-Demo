#include <stdio.h>
#include "../Shared/World.h"
#include "../Shared/Structs.h" 
#include "../Shared/Messages.h" 
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
#include <string> 

using boost::asio::ip::udp;

const int TICKRATE = 60;
const int TICK_INTERVAL = 1000 / TICKRATE;

Shared::World world;
std::unordered_map<std::string, udp::endpoint> clientEndpoints;
boost::asio::io_context io_context;
udp::socket UDPsocket = udp::socket(io_context, udp::endpoint(udp::v4(), 6666));
uint32_t tick = 0;


// Accept only types that are derived from Shared::Message (but not Message itself)
template<typename T>
    requires std::derived_from<T, Shared::Message> && (!std::same_as<T, Shared::Message>)
void sendMessage(T& message, const udp::endpoint& endpoint) {
    message.sendTick = tick;
    size_t messageSize = sizeof(T);
    UDPsocket.send_to(boost::asio::buffer(&message, messageSize), endpoint);
    //TODO ACK TRACKING
}


void handleMessage(const char* data, size_t length, const udp::endpoint& sender_endpoint) {
    if (length < sizeof(Shared::MessageType)) {
        printf("Invalid message received\n");
        return;
    }

    // Read the message type
    Shared::MessageType type = *reinterpret_cast<const Shared::MessageType*>(data);

    switch (type) {
    case Shared::MessageType::PlayerRegister: {
        if (length < sizeof(Shared::PlayerRegister)) {
            printf("Invalid PlayerRegister message size\n");
            return;
        }

        const Shared::PlayerRegister* playerRegister = reinterpret_cast<const Shared::PlayerRegister*>(data);
        printf("Player joined with ID: %s\n", playerRegister->playerId);

        std::string string_name = playerRegister->playerId;
        // Store the client endpoint
        clientEndpoints[string_name] = sender_endpoint;

        //check if player already exists
		if (world.GetPlayers().find(string_name) != world.GetPlayers().end()) {
			printf("Player already exists\n");
        }
        else {
            //add new players
            world.AddPlayer(Shared::Player(string_name, rand() % 10, rand() % 10, rand() % 255, rand() % 255, rand() % 255));
        }
       
		//now spawn all players to the newly connected client
		
       // First send all world objects
        for (const auto& worldObject : world.GetWorld()) {
            Shared::WorldSpawn msg(worldObject.pos, worldObject.color);
            sendMessage(msg, sender_endpoint);
        }

        // Then send all players (with their specific player information)
        for (const auto& [playerName, player] : world.GetPlayers()) {
            Shared::WorldSpawn msg(player);
            sendMessage(msg, sender_endpoint);
        }
        
        break;
    }


    default:
        printf("Unknown message type received\n");
        break;
    }
}

void handle_udp_server() {
    char data[1024];
    udp::endpoint sender_endpoint;

    // Loop to handle multiple packets
    while (UDPsocket.available() > 0) { // Check if there are packets available
        // Receive data (blocking call)
        size_t length = UDPsocket.receive_from(boost::asio::buffer(data), sender_endpoint);

        std::cout << "Received from " << sender_endpoint.address().to_string() << ":"
            << sender_endpoint.port() << "\n";

		handleMessage(data, length, sender_endpoint);
    }
}
int main()
{

    std::cout << "Logging test: Server started\n";



    //initialise some randomised world objects so players can be seen moving instead of being stuck in a black void
   
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(Shared::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));




    while (true) {
        tick++;
        auto tickStart = std::chrono::high_resolution_clock::now();

  
        try {
            handle_udp_server();
        }
        catch (const std::exception& e) {
            std::cerr << "UDP server error: " << e.what() << "\n";
        }
 
        world.Update(TICK_INTERVAL);

        std::vector<Shared::PlayerUpdate> playerUpdates;

		//send update for each player
        for (const auto& player : world.GetPlayers()) {
			//make a message
			Shared::PlayerUpdate update(player.second);

            playerUpdates.push_back(update);
        
        }
        for (auto& update : playerUpdates) {
            char buffer[1024];
            size_t messageSize = sizeof(update);

            // Serialize the message
            memcpy(buffer, &update, messageSize);

            // Send the message to all clients
            for (const auto& client : clientEndpoints) {
				sendMessage(update, client.second);
            }
        }

        auto tickEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tickEnd - tickStart);

        int sleepTime = TICK_INTERVAL - static_cast<int>(elapsed.count());
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
        else {
			std::cout << "Tick took too long! (" << -sleepTime << "ms over)\n";
        }
    }
}

