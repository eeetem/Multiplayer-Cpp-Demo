#include "pch.h"
#include "World.h"
#include <stdexcept>

namespace Shared {

	WorldObject::WorldObject(float x, float y, unsigned char r, unsigned char g, unsigned char b)
		: pos{ x, y }, color{ r, g, b } {
	}
    WorldObject::WorldObject() : pos{ 0, 0 }, color{ 0, 0, 0 } {
    }

    // Implementation of Player constructors
    Player::Player(std::string name, float x, float y, unsigned char r, unsigned char g, unsigned char b)
        : WorldObject(x,y,r,g,b), name(std::move(name)), vel{ 0, 0 } {
    }

    Player::Player() : WorldObject(), vel{ 0, 0 }{
    }

   
    World::World() {}
    World::~World() {}

    void World::Update(float deltaTime) {
        for (auto& [name, player] : players) {
            player.pos.x += player.vel.x * deltaTime;
            player.pos.y += player.vel.y * deltaTime;
        }
    }

    void World::AddPlayer(const Player& player) {
        players[player.name] = player;
    }
    void World::AddWorldObject(const WorldObject& wo) {
        worldObjs.push_back(wo);
    }

    Player& World::GetPlayerByName(const std::string& name) {
        auto it = players.find(name);
        if (it == players.end()) {
            throw std::out_of_range("Player with the given name does not exist");
        }
        return it->second;
    }
	bool World::PlayerExists(const std::string& name) const {
		return players.find(name) != players.end();
	}

    const std::unordered_map<std::string, Player>& World::GetPlayers() const {
        return players;
    }

    const std::vector<WorldObject>& World::GetWorld() const {
        return worldObjs; 
    }

    
}