#include <thread>
#include <chrono>
#include <iostream>
#include <ostream>
#include <map>
#include <mutex>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "websocket.hpp"
#include "display_impl.hpp"
#include "debug_util.hpp"

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


void handle_data(display::Display *disp, json data)
{
    DEBUG_LOG("data: " << data);

    disp->show("foobar");
}

void do_session(int threadId, display::Display *disp, tcp::socket *socket)
{
    auto ip = socket->remote_endpoint().address();
    auto port = socket->remote_endpoint().port();

    DEBUG_LOG("connection opened: " << ip << ":" << port);
    DEBUG_LOG("thread " << threadId);

    try
    {
        websocket::stream<tcp::socket &> ws{*socket};
        ws.accept();

        while (true)
        {
            boost::beast::multi_buffer buffer;
            ws.read(buffer);

            auto data = boost::beast::buffers_to_string(buffer.data());
            handle_data(disp, json::parse(data));
        }
    }
    catch (boost::system::system_error const &se)
    {
        if (se.code() != websocket::error::closed)
            std::cerr << "Error: " << se.code().message() << std::endl;
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    DEBUG_LOG("connection closed: " << ip << ":" << port);
}

struct Connection {
    std::thread thread;
    std::unique_ptr<tcp::socket> socket;
};

std::map<int, Connection> connections{};
std::mutex connection_mutex;

void shutdown()
{
    std::unique_lock<std::mutex> lock(connection_mutex);
    DEBUG_LOG("shutdown start");

    for (auto &connection : connections)
    {
        connection.second.socket->shutdown(tcp::socket::shutdown_both);
        connection.second.socket = nullptr;
    }

    for (auto &connection : connections)
    {
        connection.second.thread.join();
    }

    connections.clear();

    DEBUG_LOG("shutdown done");
}

int main()
{
    try
    {
        auto const address = net::ip::make_address(SERVER_HOST);
        auto const port = static_cast<unsigned short>(std::atoi(SERVER_PORT));

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};

        auto disp = std::make_unique<display::DisplayImpl>([] {}, [] {});
        disp->start();

        int threads = 0;

        while (true)
        {
            auto socket = std::make_unique<tcp::socket>(ioc);
            acceptor.accept(*socket);

            std::unique_lock<std::mutex> lock(connection_mutex);

            int threadId = threads++;
            auto thread = std::thread{std::bind(&do_session, threadId, disp.get(), socket.get())};

            Connection connection{std::move(thread),std::move(socket)};
            connections[threadId] = std::move(connection);
            break;
        }

        acceptor.close();

        DEBUG_LOG("waiting");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        shutdown();

        disp->stop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
