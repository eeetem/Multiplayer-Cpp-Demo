#include <stdio.h>
#include <Shared/World.h>
#include <Shared/Structs.h>
#include <Shared/Messages.h> 
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
#include <string> 
#include <Shared/Network.h>

using boost::asio::ip::udp;

World::World world;


int main()
{

    std::cout << "Logging test: Server started\n";



    //initialise some randomised world objects so players can be seen moving instead of being stuck in a black void
    world.AddWorldObject(World::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(World::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(World::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(World::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));
    world.AddWorldObject(World::WorldObject(rand() % 500 - 250, rand() % 500 - 250, 200, 200, 200));

    Network::InitServer(6666);




    while (true) {
     
        auto tickStart = std::chrono::high_resolution_clock::now();
 
        world.Update(Network::NETWORK_TICK_INTERVAL);
		Network::Update(world);


        auto tickEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tickEnd - tickStart);

        int sleepTime = Network::NETWORK_TICK_INTERVAL - static_cast<int>(elapsed.count());
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
        else {
			std::cout << "Tick took too long! (" << -sleepTime << "ms over)\n";
        }
	//	printf("tick took %dms\n", static_cast<int>(elapsed.count()));
    }
}

