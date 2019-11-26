#include <thread>
#include <chrono>
#include <iostream>
#include <ostream>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "websocket.hpp"
#include "display_impl.hpp"

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

void handle_data(display::DisplayImpl * disp, json data)
{
    std::cout << "data: " << data << std::endl;
    disp->show("foobar");
}

void do_session(display::DisplayImpl * disp,
    tcp::socket & socket)
{
    auto ip = socket.remote_endpoint().address();
    auto port = socket.remote_endpoint().port();
    std::cout << "connection opened: " << ip << ":" << port << std::endl;

    try
    {
        websocket::stream<tcp::socket> ws{std::move(socket)};
        ws.accept();

        while (true)
        {
            boost::beast::multi_buffer buffer;
            ws.read(buffer);

            auto data = boost::beast::buffers_to_string(buffer.data());
            handle_data(disp, json::parse(data));
        }
    }
    catch(boost::system::system_error const& se)
    {
        if(se.code() != websocket::error::closed)
            std::cerr << "Error: " << se.code().message() << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "connection closed: " << ip << ":" << port << std::endl;
}

int main()
{
    try
    {
        auto const address = net::ip::make_address(SERVER_HOST);
        auto const port = static_cast<unsigned short>(std::atoi(SERVER_PORT));

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};

        auto disp = std::make_unique<display::DisplayImpl>([]{}, []{});
        disp->start();

        while (true)
        {
            tcp::socket socket{ioc};
            acceptor.accept(socket);

            std::thread{std::bind(&do_session, disp.get(), std::move(socket))}
                .detach();
        }

        disp->stop();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
