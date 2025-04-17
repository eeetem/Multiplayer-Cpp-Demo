#include "World.h"
#include <stdexcept>

namespace World
{
    WorldObject::WorldObject(float x, float y, unsigned char r, unsigned char g, unsigned char b)
        : pos{x, y}, color{r, g, b}
    {
    }

    WorldObject::WorldObject() : pos{0, 0}, color{0, 0, 0}
    {
    }

    Player::Player(std::string name, float x, float y, unsigned char r, unsigned char g, unsigned char b)
        : WorldObject(x, y, r, g, b), vel{0, 0}, name(std::move(name))
    {
    }

    Player::Player() : WorldObject(), vel{0, 0}
    {
    }


    World::World()
    {
    }

    World::~World()
    {
    }

    void World::Update(float deltaTime)
    {
        for (auto& [name, player] : players)
        {
            player.Update(deltaTime);
        }
    }

    void Player::Update(float deltaTime)
    {
        if (vel.x != 0 || vel.y != 0)
        {
            dirty = true;
        }

        pos += vel * deltaTime;
        // vel.x = 0;
        // vel.y = 0;

        //TODO damp velocity for players the current client isnt controling to avoid jittering
    }

    void World::AddPlayer(const Player& player)
    {
        players[player.name] = player;
    }

    void World::AddWorldObject(const WorldObject& wo)
    {
        worldObjs.push_back(wo);
    }

    Player& World::GetPlayerByName(const std::string& name)
    {
        auto it = players.find(name);
        if (it == players.end())
        {
            throw std::out_of_range("Player with the given name does not exist");
        }
        return it->second;
    }

    bool World::PlayerExists(const std::string& name) const
    {
        return players.contains(name);
    }

    const std::unordered_map<std::string, Player>& World::GetPlayers() const
    {
        return players;
    }

    const std::vector<WorldObject>& World::GetWorld() const
    {
        return worldObjs;
    }
}
