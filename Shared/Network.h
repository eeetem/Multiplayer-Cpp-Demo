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


namespace Network
{
    constexpr int NETWORK_TICKRATE = 60;
    constexpr int NETWORK_TICK_INTERVAL = 1000 / NETWORK_TICKRATE;
    constexpr int PACKET_LOSS_SIM = 15;
    constexpr int LATENCY_MIN_SIM = 10;
    constexpr int LATENCY_MAX_SIM = 30;


    extern boost::asio::io_context io_context;
    extern udp::socket socket;
    extern uint32_t tick;
    extern uint16_t nextMessageId;

    struct PendingAckMessage
    {
        udp::endpoint endpoint;
        uint32_t lastAttemptTick;
        std::vector<char> data;
    };

    extern std::map<uint16_t, PendingAckMessage> ackMessages;


#ifdef SERVER
    struct Client
    {
        std::string name;
        uint32_t lastClientTick = 0;
        uint32_t lastPositionUpdateServerTick = 0;
    };

    extern std::map<udp::endpoint, Client> clients;
    void InitServer(int port);
#endif

#ifdef CLIENT
    void InitClient(const char* username, std::string ip = "127.0.0.1", int port = 6666);
    void SendPlayerUpdate(World::Player& p);
#endif

    void DeliverMessage(boost::asio::mutable_buffer data, const udp::endpoint& endpoint);

    template <typename T>
        requires std::derived_from<T, Message> && (!std::same_as<T, Message>)
    void ProcessAndSendMessage(T& message, uint32_t tick, const udp::endpoint& endpoint)
    {
        message.sendTick = tick;
        size_t messageSize = sizeof(T);
        if (message.sendType == MessageSendType::Guaranteed)
        {
            message.msgID = nextMessageId;
            nextMessageId++;

            // Create a copy of the message data
            std::vector<char> data_copy(messageSize);
            memcpy(data_copy.data(), &message, messageSize);

            ackMessages[message.msgID] = PendingAckMessage{
                endpoint,
                tick,
                std::move(data_copy)
            };
            printf("Sending guaranteed message with ID %d\n", message.msgID);
        }
        else
        {
            message.msgID = 0;
            printf("Sending ordered fast message\n");
        }
        DeliverMessage(boost::asio::buffer(&message, messageSize), endpoint);
    }

    void RecieveMessage(const char* data, size_t length, World::World& world, const udp::endpoint& sender_endpoint);

    void HandleMessage(const char* data, MessageType type, World::World& world, const udp::endpoint& sender_endpoint);


    void Update(World::World& world);
}
