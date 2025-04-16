#include <stdio.h>
#include "World.h"
#include "Structs.h" 
#include "Messages.h" 
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
#include <map>
#include "Network.h"


using boost::asio::ip::udp;

namespace Network {

	boost::asio::io_context io_context;
	udp::socket socket(io_context);
	uint32_t tick = 0;

#if CLIENT
	udp::endpoint server_endpoint;
	uint32_t lastRecivedTick = 0;
	PlayerUpdate lastUpdate;
	static auto lastPlayerUpdate = std::chrono::steady_clock::now();
	bool connected = false;
#else
	std::map<udp::endpoint, Client> clients; 
#endif


#ifdef SERVER
	void InitServer(int port) {
		socket = udp::socket(io_context, udp::endpoint(udp::v4(), port));
		printf("Server started on port %d\n", port);
	}

	void HandleMessage(const char* data, MessageType type, World::World& world, const udp::endpoint& sender_endpoint) {
		switch (type) {
		case MessageType::WorldSpawn: {
			//server should not recive this ever
			printf("Recived World Spawn on server, dropping....\n");
			break;
		}
		case MessageType::PlayerUpdate: {
			const PlayerUpdate* update = reinterpret_cast<const PlayerUpdate*>(data);


			Client& c = clients[sender_endpoint];
			//check authority
			if (c.name != update->playerId) {
				printf("Player tried to update another player, dropping....\n");
				break;
			}
			if (!world.PlayerExists(update->playerId)) {
				printf("Player character missing, dropping....\n");
				break;
			}
			//if (tick == c.lastPositionUpdateServerTick) break;


			World::Player& p = world.GetPlayerByName(update->playerId);

			//check if player can reach here since their last update
			float possibleDistanceSinceLastUpdate = World::PLAYER_SPEED * (tick - c.lastPositionUpdateServerTick) * NETWORK_TICK_INTERVAL;
			float distance = sqrt(pow(update->pos.x - p.pos.x, 2) + pow(update->pos.y - p.pos.y, 2));

			//printf("Player moved from %f,%f to %f,%f\n", p.pos.x, p.pos.y, update->pos.x, update->pos.y);

			if (distance > possibleDistanceSinceLastUpdate) {
				printf("Max move distnace is %f, player moved %f\n", possibleDistanceSinceLastUpdate, distance);
				// Set as close as possible to the requested position while respecting max speed
				float moveDistance = possibleDistanceSinceLastUpdate;
				float dirX = update->pos.x - p.pos.x;
				float dirY = update->pos.y - p.pos.y;
				printf("dir: %f,%f\n", dirX, dirY);
				float dirLength = sqrt(dirX * dirX + dirY * dirY);

				if (dirLength > 0) {
					// Normalize direction vector
					dirX /= dirLength;
					dirY /= dirLength;

					// Move player forward along this direction by the max allowed distance
					p.pos.x = p.pos.x + dirX * moveDistance;
					p.pos.y = p.pos.y + dirY * moveDistance;
				}
				// Update velocity based on the direction of attempted movement
				p.vel.x = update->vel.x;
				p.vel.y = update->vel.y;

				printf("Player %s attempted to move too far! Capped at (%f, %f)\n",
					update->playerId, p.pos.x, p.pos.y);
			}
			else {
				p.pos.x = update->pos.x;
				p.pos.y = update->pos.y;
				p.vel.x = update->vel.x;
				p.vel.y = update->vel.y;
				printf("Player %s updated to (%f, %f)\n", update->playerId, p.pos.x, p.pos.y);
			}
			c.lastPositionUpdateServerTick = tick;
			p.dirty = true;

			break;
		}
		case MessageType::PlayerRegister: {
			const PlayerRegister* playerRegister = reinterpret_cast<const PlayerRegister*>(data);
			printf("Player joined with ID: %s\n", playerRegister->playerId);

			std::string string_name = playerRegister->playerId;

			// Store the client in our map using endpoint as key
			

			//check if player already exists
			if (world.GetPlayers().find(string_name) != world.GetPlayers().end()) {
				printf("Player already exists\n");
				return;//connection refused
			}
			else {
				//add new players
				world.AddPlayer(World::Player(string_name, rand() % 10, rand() % 10, rand() % 255, rand() % 255, rand() % 255));
			}
			clients[sender_endpoint] = Client{ string_name, tick };
			//echo back to confirm connection
			PlayerRegister msg(playerRegister->playerId);
			SendMessage(msg, tick, sender_endpoint);
			
			
			
			
			//now spawn all players to the newly connected client

			// First send all world objects
			for (const auto& worldObject : world.GetWorld()) {
				WorldSpawn msg(worldObject.pos, worldObject.color);
				SendMessage(msg, tick, sender_endpoint);
			}

			// Then send all players (with their specific player information)
			for (const auto& [playerName, player] : world.GetPlayers()) {
				WorldSpawn msg(player);
				SendMessage(msg, tick, sender_endpoint);
			}
			break;
		}


		default:
			printf("Unknown message type received\n");
			break;
		}
	}
#else

	void InitClient(const char* username, std::string ip, int port) {
		socket = udp::socket(io_context, udp::endpoint(udp::v4(), 0));
		server_endpoint = udp::endpoint(boost::asio::ip::make_address(ip), port);
		connected = false;
		//a random int is used but if i bothered to implement a gui this could be a username

		printf("connecting with %s\n", username);
		PlayerRegister playerRegister(username);
		socket.send_to(boost::asio::buffer(&playerRegister, sizeof(playerRegister)), server_endpoint);
	}

	
	void SendPlayerUpdate(World::Player& p) {
		

		// Check if enough time has passed since the last update (based on NETWORK_TICK_INTERVAL) there's no point in updating the sever multiple times per network tick
		auto currentTime = std::chrono::steady_clock::now();
		auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastPlayerUpdate).count();

		if (elapsedMs < NETWORK_TICK_INTERVAL) {
			return;
		}
		PlayerUpdate update(p);
		// Also skip if change in position is less than 1 tick's worth of movement
		float distance = sqrt(pow(update.pos.x - lastUpdate.pos.x, 2) + pow(update.pos.y - lastUpdate.pos.y, 2));
		if (distance < World::PLAYER_SPEED*NETWORK_TICK_INTERVAL) {
			return;
		}
		SendMessage(update, tick, server_endpoint);
		lastUpdate = update;
		lastPlayerUpdate = currentTime;
	}

	void HandleMessage(const char* data, MessageType type, World::World& world, const udp::endpoint& sender_endpoint) {

		switch (type) {
		case MessageType::PlayerRegister: {
				//server echoes this back when confirming connection
				connected = true;
				break;
			}
		case MessageType::WorldSpawn: {
			const WorldSpawn* spawn = reinterpret_cast<const WorldSpawn*>(data);
			if (spawn->player) {
				world.AddPlayer(World::Player(spawn->playerId, spawn->pos.x, spawn->pos.y, spawn->color.r, spawn->color.g, spawn->color.b));
				printf("Player spawned with ID: %s\n", spawn->playerId);
			}
			else {
				world.AddWorldObject(World::WorldObject(spawn->pos.x, spawn->pos.y, spawn->color.r, spawn->color.g, spawn->color.b));
				printf("World object spawned at (%f, %f)\n", spawn->pos.x, spawn->pos.y);
			}
			break;
		}
		case MessageType::PlayerUpdate: {
			const PlayerUpdate* update = reinterpret_cast<const PlayerUpdate*>(data);
			if (world.PlayerExists(update->playerId)) {

				World::Player& p = world.GetPlayerByName(update->playerId);
				p.pos.x = update->pos.x;
				p.pos.y = update->pos.y;
				p.vel.x = update->vel.x;
				p.vel.y = update->vel.y;
				printf("Player %s updated to (%f, %f)\n", update->playerId, p.pos.x, p.pos.y);
			}
			break;
		}

		default:
			printf("Unknown message type received\n");
			break;
		}
	}
#endif

	void RecieveMessage(const char* data, size_t length, World::World& world, const udp::endpoint & sender_endpoint) {
		if (length < sizeof(MessageType)) {
			printf("Invalid message received\n");
			return;
		}


		// Read the message type
		MessageType type = *reinterpret_cast<const MessageType*>(data);
		uint32_t tick = *reinterpret_cast<const uint32_t*>(data + sizeof(MessageType));
		printf("Message type %d from tick %d\n", static_cast<int>(type), tick);
	
		uint32_t& lastRecived = tick;// In case server fails to match client(ie this is a new client) just set the tick to the current one
#ifdef CLIENT
		lastRecived = lastRecivedTick;
#elif defined(SERVER)
		if (clients.find(sender_endpoint) != clients.end()) {
			lastRecived = clients[sender_endpoint].lastClientTick;
		}
#endif
		if (tick >= lastRecived) {
			lastRecived = tick;
		}
		else {
			printf("Out of order message, dropping....\n");
			return;
		}

		if (length < sizeof(type)) {
			printf("Invalid message size received\n");
			return;
		}
		HandleMessage(data, type, world, sender_endpoint);
	}


	void Update(World::World& world) {
		char data[1024];
		udp::endpoint sender_endpoint;
		try {
			while (socket.available() > 0) { // Check if there are packets available
			
				size_t length = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
				std::cout << "Received from " << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << "\n";
				RecieveMessage(data, length, world, sender_endpoint);
			
			}
		}
		catch (const boost::system::system_error& e) {
			// Handle boost asio errors
			std::cerr << "Network error: " << e.what() << std::endl;
		}
		tick++;


#ifdef SERVER
		//player updtes
		std::vector<Network::PlayerUpdate> playerUpdates;

		for (auto& player : world.GetPlayers()) {
			if (player.second.dirty) {
				Network::PlayerUpdate update(player.second);
				playerUpdates.push_back(update);
				world.GetPlayerByName(player.second.name).dirty = false;
			}

		}
		for (auto& update : playerUpdates) {
			char buffer[1024];
			size_t messageSize = sizeof(update);

			for (const auto& client : clients) {
				SendMessage(update, tick, client.first);
			}
		}
#endif
	 }

}