#include <Shared/Messages.h>
#include <Shared/Network.h>
#include "Server.h"


namespace Network {


	

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
			
			
			World::Player& p = world.GetPlayerByName(update->playerId);
			
			//check if player can reach here since their last update
			float possibleDistanceSinceLastUpdate = World::PLAYER_SPEED * (tick - c.lastPositionUpdateServerTick) * TICK_INTERVAL;
			float distance = sqrt(pow(update->pos.x - c.lastUpdatePostion.x, 2) + pow(update->pos.y - c.lastUpdatePostion.y, 2));
			
			if (distance > possibleDistanceSinceLastUpdate) {
				printf("Max move distnace is %f, player moved %f\n", possibleDistanceSinceLastUpdate, distance);
				// Set as close as possible to the requested position while respecting max speed
				float angle = atan2(update->pos.y - c.lastUpdatePostion.y, update->pos.x - c.lastUpdatePostion.x);

				// Calculate the max allowed position along the same direction
				p.pos.x = c.lastUpdatePostion.x + cos(angle) * possibleDistanceSinceLastUpdate;
				p.pos.y = c.lastUpdatePostion.y + sin(angle) * possibleDistanceSinceLastUpdate;

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

			c.lastUpdatePostion = p.pos;
			c.lastPositionUpdateServerTick = tick;
			p.dirty = true;
			
			break;
		}
		case MessageType::PlayerRegister: {
			const PlayerRegister* playerRegister = reinterpret_cast<const PlayerRegister*>(data);
			printf("Player joined with ID: %s\n", playerRegister->playerId);

			std::string string_name = playerRegister->playerId;

			// Store the client in our map using endpoint as key
			clients[sender_endpoint] = Client{ string_name, tick};

			//check if player already exists
			if (world.GetPlayers().find(string_name) != world.GetPlayers().end()) {
				printf("Player already exists\n");
			}
			else {
				//add new players
				world.AddPlayer(World::Player(string_name, rand() % 10, rand() % 10, rand() % 255, rand() % 255, rand() % 255));
			}

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

}