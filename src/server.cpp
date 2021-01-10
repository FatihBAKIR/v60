#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <v60/server.hpp>

namespace v60 {
namespace {
using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>

void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}
} // namespace

struct server_impl {
    any_routable<request<object<>, std::string_view>, any_response> m_route;
    net::io_context m_ioc{4};

    server_impl(any_routable<request<object<>, std::string_view>, any_response> route)
        : m_route{std::move(route)} {
    }

    template<class Body, class Allocator, class Send>
    v60::task<void>
    handle_request(beast::http::request<Body, beast::http::basic_fields<Allocator>>&& req,
                   Send&& send) {
        // Returns a bad request response
        auto const bad_request = [&req](beast::string_view why) {
            beast::http::response<beast::http::string_body> res{
                beast::http::status::bad_request, req.version()};
            res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(beast::http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

        // Request path must be absolute and not contain "..".
        if (req.target().empty() || req.target()[0] != '/' ||
            req.target().find("..") != beast::string_view::npos) {
            co_await send(bad_request("Illegal request-target"));
            co_return;
        }

        auto body = req.body();
        const auto target = std::string(req.target());
        const auto verb = req.method();
        auto reqq = request<v60::object<>, std::string_view>{
            base_request{std::move(req)}, {}, body};

        beast::http::response<beast::http::string_body> res;
        res.set(beast::http::field::server, "v60_over_" BOOST_BEAST_VERSION_STRING);
        res.version(reqq.version());
        res.keep_alive(req.keep_alive());

        v60::any_response resp(send, std::move(res));
        const auto route_res = m_route.match(verb, target) &&
                               co_await m_route(std::move(reqq), std::move(resp));

        if (route_res) {
            std::cerr << "200 [" << to_string(verb) << "] \"" << target << "\"\n";
        }
    }

    net::awaitable<void> do_session(beast::tcp_stream stream) {
        bool close = false;

        auto lambda = [&]<bool isRequest, class Body, class Fields>(
                          beast::http::message<isRequest, Body, Fields> msg)
            -> net::awaitable<void> {
            close = msg.need_eof();

            // We need the serializer here because the serializer requires
            // a non-const file_body, and the message oriented version of
            // http::write only works with const messages.
            beast::http::serializer<isRequest, Body, Fields> sr{msg};
            co_await beast::http::async_write(stream, sr, net::use_awaitable);
            //        http::write(stream, sr);
        };

        beast::flat_buffer buffer;

        try {

            for (;;) {
                beast::http::request<beast::http::string_body> req;

                co_await beast::http::async_read(stream, buffer, req, net::use_awaitable);

                co_await handle_request(std::move(req), lambda);
                if (close) {
                    break;
                }
            }
        } catch (std::exception& err) {
            std::cerr << err.what() << '\n';
        }

        // Send a TCP shutdown
        stream.socket().shutdown(tcp::socket::shutdown_send);
    }

    net::awaitable<void> do_listen(net::io_context& ioc, tcp::endpoint endpoint) {
        beast::error_code ec;

        std::cerr << "Listening...\n";

        // Open the acceptor
        tcp::acceptor acceptor(ioc);
        acceptor.open(endpoint.protocol(), ec);
        if (ec)
            co_return fail(ec, "open");

        // Allow address reuse
        acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
            co_return fail(ec, "set_option");

        // Bind to the server address
        acceptor.bind(endpoint, ec);
        if (ec)
            co_return fail(ec, "bind");

        // Start listening for connections
        acceptor.listen(net::socket_base::max_listen_connections, ec);
        if (ec)
            co_return fail(ec, "listen");

        for (;;) {
            tcp::socket socket(ioc);
            co_await acceptor.async_accept(socket, boost::asio::use_awaitable);
            std::cerr << "Got connection\n";

            boost::asio::co_spawn(ioc,
                                  do_session(beast::tcp_stream(std::move(socket))),
                                  boost::asio::detached);
            if (ec)
                fail(ec, "accept");
        }
    }

    void listen(int p_port) {
        auto const address = net::ip::make_address("0.0.0.0");
        auto const port = static_cast<unsigned short>(p_port);
        auto const threads = std::max<int>(1, 4);

        boost::asio::co_spawn(
            m_ioc, do_listen(m_ioc, tcp::endpoint{address, port}), boost::asio::detached);

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for (auto i = threads - 1; i > 0; --i) v.emplace_back([this] { m_ioc.run(); });
        m_ioc.run();

        for (auto& t : v) {
            t.join();
        }
    }
};

server::server(any_routable<request<object<>, std::string_view>, any_response> root_route)
    : m_impl(std::make_unique<server_impl>(std::move(root_route))) {
}

void server::listen(int p_port) {
    m_impl->listen(p_port);
}

server::~server() = default;
} // namespace v60