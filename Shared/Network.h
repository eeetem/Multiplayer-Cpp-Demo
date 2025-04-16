#pragma once
#include <concepts>
#include <map>
#include <string>
#include "Messages.h"
#include <boost/asio.hpp>
using boost::asio::ip::udp;

#ifndef CLIENT
    #ifndef SERVER
        #error "Either CLIENT or SERVER must be defined."
    #endif
#endif


namespace Network {


    const int NETWORK_TICKRATE = 60;
    const int NETWORK_TICK_INTERVAL = 1000 / NETWORK_TICKRATE;


    extern boost::asio::io_context io_context;
    extern udp::socket socket;
    extern uint32_t tick;



#ifdef SERVER 
    struct Client {
        std::string name;
        uint32_t lastClientTick = 0;
        uint32_t lastPositionUpdateServerTick = 0;
    };
    extern std::map<udp::endpoint, Client> clients;
    void InitServer(int port);
#endif

    // Client-specific functionality
#ifdef CLIENT
    void InitClient(const char* username, std::string ip = "127.0.0.1", int port = 6666);
    void SendPlayerUpdate(World::Player& p);
#endif


    template<typename T>
        requires std::derived_from<T, Message> && (!std::same_as<T, Message>)
    void SendMessage(T& message, uint32_t tick, const udp::endpoint& endpoint) {
        message.sendTick = tick;
        size_t messageSize = sizeof(T);
        socket.send_to(boost::asio::buffer(&message, messageSize), endpoint);
        //TODO ACK TRACKING
    }
    void RecieveMessage(const char* data, size_t length, World::World& world, const udp::endpoint& sender_endpoint);

    void HandleMessage(const char* data, MessageType type, World::World& world, const udp::endpoint& sender_endpoint);

    void Update(World::World& world);
}