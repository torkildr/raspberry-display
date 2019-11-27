#include <thread>
#include <chrono>
#include <iostream>
#include <ostream>

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

void do_session(display::Display *disp, tcp::socket *socket)
{
    auto ip = socket->remote_endpoint().address();
    auto port = socket->remote_endpoint().port();

    DEBUG_LOG("connection opened: " << ip << ":" << port);

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

        std::vector<std::unique_ptr<tcp::socket>> sockets{};

        while (true)
        {
            auto socket = std::make_unique<tcp::socket>(ioc);
            acceptor.accept(*socket);

            auto thread = std::thread{std::bind(&do_session, disp.get(), socket.get())};
            thread.detach();

            sockets.push_back(std::move(socket));
        }

        DEBUG_LOG("shutting down");
        disp->stop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
