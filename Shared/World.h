#pragma once
#include "Structs.h"
#include <vector>
#include <unordered_map>
#include <string>


namespace World {
    constexpr int PLAYER_SIZE = 50;
    constexpr float PLAYER_SPEED = 0.35f;

    class WorldObject {
    public:
        Vector2 pos;
        Color color;

        WorldObject(float x, float y, unsigned char r, unsigned char g, unsigned char b);
        WorldObject();
        virtual ~WorldObject() = default;
    };
    class Player : public WorldObject {
    public:
        Vector2 vel;
		std::string name;
		bool dirty = false;

        Player(std::string name, float x, float y, unsigned char r, unsigned char g, unsigned char b);
        Player();
    };





    class World {
    public:
        World();
        ~World();

        void Update(float deltaTime);
        void AddPlayer(const Player& player);
        void AddWorldObject(const WorldObject& wo);
        Player& GetPlayerByName(const std::string& name);
		bool PlayerExists(const std::string& name) const;
        const std::unordered_map<std::string, Player>& GetPlayers() const;
		const std::vector<WorldObject>& GetWorld() const;

    private:
        std::unordered_map<std::string, Player> players;
        std::vector<WorldObject> worldObjs;

    };
}