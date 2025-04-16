#pragma once
#include "World.h"
#include <chrono>
#include <cstring>


namespace Network {

    enum class MessageSendType {
        Fast,
        Ack,
    };

    enum class MessageType {
        PlayerRegister = 0,
        PlayerUpdate = 1,
        WorldSpawn = 3,

    };

    struct Message {
        MessageType type;
        uint32_t sendTick = -1;

        MessageSendType sendType = MessageSendType::Fast;
        uint16_t msgID = 0; // Message ID for tracking

        Message(MessageType messageType)
            : type(messageType) {
        }
    };

    struct PlayerRegister : public Message {
        char playerId[32];

        PlayerRegister(const char* id) : Message(MessageType::PlayerRegister) {
            strncpy_s(playerId, id, sizeof(playerId) - 1);
            playerId[sizeof(playerId) - 1] = '\0';
        }
    };



	struct WorldSpawn : public Message {
		World::Vector2 pos;
        World::Color color;

        bool player;
        char playerId[32];

		WorldSpawn() : Message(MessageType::WorldSpawn), pos{ 0, 0 }, color{ 0, 0, 0 }, player(false) {
		}

		WorldSpawn(const World::Vector2& position, const World::Color& color)
			: Message(MessageType::WorldSpawn), pos(position), color(color), player(false) {
		}

        WorldSpawn(const World::Player& p)
            : Message(MessageType::WorldSpawn), pos(p.pos), color(p.color), player(true) {
            strncpy_s(playerId, p.name.c_str(), sizeof(playerId) - 1);
            playerId[sizeof(playerId) - 1] = '\0';
        }
	};

    struct PlayerUpdate : public Message {
        char playerId[32];
        World::Vector2 pos;
        World::Vector2 vel;

        // Constructor for PlayerUpdate
        PlayerUpdate(const char* id, const World::Vector2& position, const World::Vector2& velocity)
            : Message(MessageType::PlayerUpdate), pos(position), vel(velocity) {
            strncpy_s(playerId, id, sizeof(playerId) - 1);
            playerId[sizeof(playerId) - 1] = '\0'; 
        }

        // Constructor from Player object
        PlayerUpdate(const World::Player& p)
            : Message(MessageType::PlayerUpdate), pos(p.pos), vel(p.vel) {
            strncpy_s(playerId, p.name.c_str(), sizeof(playerId) - 1);
            playerId[sizeof(playerId) - 1] = '\0';
        }
		PlayerUpdate() :Message(MessageType::PlayerUpdate), pos{ 0,0 }, vel{ 0,0 } {
		}
    };
}