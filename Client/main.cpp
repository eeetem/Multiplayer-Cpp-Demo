#include <SDL.h>
#include <stdio.h>
#include "../Shared/World.h"
#include "../Shared/Structs.h" 
#include "../Shared/Messages.h" 

#include <boost/asio.hpp>


using boost::asio::ip::udp;


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

Shared::World world;

void close()
{
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_Quit();
}

uint32_t tick_recived = 0;
void handleMessage(const char* data, size_t length) {
	if (length < sizeof(Shared::MessageType)) {
		printf("Invalid message received\n");
		return;
	}

	// Read the message type
	Shared::MessageType type = *reinterpret_cast<const Shared::MessageType*>(data);
	uint32_t tick = *reinterpret_cast<const uint32_t*>(data + sizeof(Shared::MessageType));
	printf("Message type %d from tick %d\n", static_cast<int>(type), tick);
	// Read tick sent
	if (tick >= tick_recived) {
		tick_recived = tick;
	}
	else {
		printf("Out of order message, dropping....\n");
		return;
	}


	switch (type) {
		case Shared::MessageType::WorldSpawn: {
			const Shared::WorldSpawn* spawn = reinterpret_cast<const Shared::WorldSpawn*>(data);
			if (spawn->player) {
				world.AddPlayer(Shared::Player(spawn->playerId, spawn->pos.x, spawn->pos.y, spawn->color.r, spawn->color.g, spawn->color.b));
				printf("Player spawned with ID: %s\n", spawn->playerId);
			}
			else {
				world.AddWorldObject(Shared::WorldObject(spawn->pos.x, spawn->pos.y, spawn->color.r, spawn->color.g, spawn->color.b));
				printf("World object spawned at (%f, %f)\n", spawn->pos.x, spawn->pos.y);
			}

		
		}


		default:
			printf("Unknown message type received\n");
			break;
	}
}

void updateNetwork(udp::socket& socket) {
	char data[1024];
	udp::endpoint sender_endpoint;

	// Non-blocking receive
	while (socket.available() > 0) {
		size_t length = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
		handleMessage(data, length);
	}
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
	printf("Initialised!\n");

	bool quit = false;



	SDL_Event e;
	Shared::Vector2 camera = { 0, 0 };




	Uint32 lastTime = SDL_GetTicks();
	float deltaTime = 0.0f;

	boost::asio::io_context io_context;
	udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
	udp::endpoint server_endpoint(boost::asio::ip::make_address("127.0.0.1"), 6666);


	//a random int is used but if i bothered to implement a gui this could be a username
	std::string connectID = std::to_string(rand() % 100);
	printf("connecting with %s\n", connectID.c_str());
	Shared::PlayerRegister playerRegister(connectID.c_str());
	socket.send_to(boost::asio::buffer(&playerRegister, sizeof(playerRegister)), server_endpoint);


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
			Shared::Player p = world.GetPlayerByName(connectID);
			float moveSpeed = 10.0f; // Movement speed in units per second
			if (currentKeyStates[SDL_SCANCODE_UP])
			{
				p.pos.y -= moveSpeed * deltaTime; // Move up
			}
			if (currentKeyStates[SDL_SCANCODE_DOWN])
			{
				p.pos.y += moveSpeed * deltaTime; // Move down
			}
			if (currentKeyStates[SDL_SCANCODE_LEFT])
			{
				p.pos.x -= moveSpeed * deltaTime; // Move left
			}
			if (currentKeyStates[SDL_SCANCODE_RIGHT])
			{
				p.pos.x += moveSpeed * deltaTime; // Move right
			}
		}
		//LOGIC
		updateNetwork(socket);
		world.Update(deltaTime);


		//RENDER
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		//set camera position to our player
		if (world.PlayerExists(connectID)) {
			Shared::Player p = world.GetPlayerByName(connectID);
			camera.x = p.pos.x - SCREEN_WIDTH / 2 + Shared::PLAYER_SIZE / 2;
			camera.y = p.pos.y - SCREEN_HEIGHT / 2 + Shared::PLAYER_SIZE / 2;
		}
		


	
		for (const auto& wo : world.GetWorld())
		{
	
			SDL_SetRenderDrawColor(renderer, wo.color.r, wo.color.g, wo.color.b, 255);

			int centerX = static_cast<int>(wo.pos.x) - camera.x + Shared::PLAYER_SIZE / 2;
			int centerY = static_cast<int>(wo.pos.y) - camera.y + Shared::PLAYER_SIZE / 2;
			int radius = Shared::PLAYER_SIZE / 2;

			DrawCircle(renderer, centerX, centerY, radius);
		}
		for (const auto& kv : world.GetPlayers())
		{

			auto wo = kv.second;
			SDL_SetRenderDrawColor(renderer, wo.color.r, wo.color.g, wo.color.b, 255);

			int centerX = static_cast<int>(wo.pos.x) - camera.x + Shared::PLAYER_SIZE / 2;
			int centerY = static_cast<int>(wo.pos.y) - camera.y + Shared::PLAYER_SIZE / 2;
			int radius = Shared::PLAYER_SIZE / 2;

			DrawCircle(renderer, centerX, centerY, radius);
		}


		//Update screen
		SDL_RenderPresent(renderer);
	}

	close();
	return 0;
}
