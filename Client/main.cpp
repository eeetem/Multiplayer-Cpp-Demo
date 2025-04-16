#include <SDL.h>
#include <stdio.h>
#include <Shared/World.h>
#include <Shared/Structs.h>
#include <Shared/Messages.h> 
#include <Shared/Network.h> 
#include <boost/asio.hpp>

using boost::asio::ip::udp;


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

World::World world;

void close()
{
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_Quit();
}



bool init()
{

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
	{
		printf("Warning: Linear texture filtering not enabled!");
	}

	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (renderer == NULL)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	return true;
}


// Function to draw a filled circle
void DrawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius)
{
	for (int w = 0; w < radius * 2; w++)
	{
		for (int h = 0; h < radius * 2; h++)
		{
			int dx = radius - w; // Horizontal offset
			int dy = radius - h; // Vertical offset
			if ((dx * dx + dy * dy) <= (radius * radius))
			{
				SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
			}
		}
	}
}

int main(int argc, char* args[])
{

	if (!init())
	{
		printf("Failed to initialize!\n");
		return 1;
	}
	printf("Initialised Window!\n");
	
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	std::string connectID = std::to_string(rand() % 100);
	Network::InitClient(connectID.c_str());

	bool quit = false;
	SDL_Event e;
	World::Vector2 camera = { 0, 0 };




	Uint32 lastTime = SDL_GetTicks();
	float deltaTime = 0.0f;


	while (!quit)
	{
		// Calculate delta time
		Uint32 currentTime = SDL_GetTicks();
		deltaTime = (currentTime - lastTime);
		lastTime = currentTime;
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
	
			
		}
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

	
		if (world.PlayerExists(connectID)) {
			World::Player& p = world.GetPlayerByName(connectID);
			float moveSpeed = World::PLAYER_SPEED;
			p.vel = { 0, 0 };
			if (currentKeyStates[SDL_SCANCODE_UP])
			{
				p.vel.y -= moveSpeed;
			}
			if (currentKeyStates[SDL_SCANCODE_DOWN])
			{
				p.vel.y += moveSpeed;
			}
			if (currentKeyStates[SDL_SCANCODE_LEFT])
			{
				p.vel.x -= moveSpeed;
			}
			if (currentKeyStates[SDL_SCANCODE_RIGHT])
			{
				p.vel.x += moveSpeed;
			}
			//movement packet
			Network::SendPlayerUpdate(p);
			
		}
		//LOGIC

		world.Update(deltaTime);
		Network::Update(world);

		//RENDER
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		//set camera position to our player
		if (world.PlayerExists(connectID)) {
			World::Player p = world.GetPlayerByName(connectID);
			camera.x = p.pos.x - SCREEN_WIDTH / 2 + World::PLAYER_SIZE / 2;
			camera.y = p.pos.y - SCREEN_HEIGHT / 2 + World::PLAYER_SIZE / 2;
		}
		


		for (const auto& [name, player] : world.GetPlayers())
		{
			SDL_SetRenderDrawColor(renderer, player.color.r, player.color.g, player.color.b, 255);

			int centerX = static_cast<int>(player.pos.x) - camera.x + World::PLAYER_SIZE / 2;
			int centerY = static_cast<int>(player.pos.y) - camera.y + World::PLAYER_SIZE / 2;
			int radius = World::PLAYER_SIZE / 2;

			DrawCircle(renderer, centerX, centerY, radius);
		}
		for (const auto& wo : world.GetWorld())
		{
			SDL_SetRenderDrawColor(renderer, wo.color.r, wo.color.g, wo.color.b, 255);

			int centerX = static_cast<int>(wo.pos.x) - camera.x + World::PLAYER_SIZE / 2;
			int centerY = static_cast<int>(wo.pos.y) - camera.y + World::PLAYER_SIZE / 2;
			int radius = World::PLAYER_SIZE / 2;

			DrawCircle(renderer, centerX, centerY, radius);
		}


		//Update screen
		SDL_RenderPresent(renderer);
	}

	close();
	return 0;
}
