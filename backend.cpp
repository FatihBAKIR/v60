#include <algorithm>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <concepts>
#include <coro_concepts.hpp>
#include <ctre.hpp>
#include <iostream>
#include <v60/async.hpp>
#include <v60/binding.hpp>
#include <v60/end_point.hpp>
#include <v60/fixed_string.hpp>
#include <v60/group.hpp>
#include <v60/middleware.hpp>
#include <v60/request.hpp>
#include <v60/response.hpp>

auto user_router() {
    using namespace v60;

    auto name_handler = [](const Request auto& req, Response auto&& resp) -> task<void> {
        co_await resp.send("hello");
        req.params.for_each_member([&]<auto key>(auto& val) {
            std::cerr << std::string_view(key) << ": " << val << '\n';
        });
    };

    auto age_handler = [](const Request auto& req, Response auto&& resp) -> task<void> {
        co_await resp.send("age: 1234");

        //        std::cerr << req.body << '\n';
        req.body.for_each_member([&]<auto k>(auto& val) {
            std::cerr << std::string_view(k) << ": " << val << '\n';
        });
        req.params.for_each_member([&]<auto key>(auto& val) {
            std::cerr << std::string_view(key) << ": " << val << '\n';
        });
    };

    return group(get<"/name">(name_handler),
                 use(object_body<object<member<"age", int>>>, post<"/age">(age_handler)));
}

auto app() {
    using namespace v60;
    return bind<"/user/:userId">(user_router());
}

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>

template<class Body, class Allocator, class Send>
v60::task<void> handle_request(http::request<Body, http::basic_fields<Allocator>>&& req,
                               Send&& send) {
    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
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

    auto not_found_mw = [&](v60::Request auto req,
                            v60::Response auto resp,
                            v60::Routable auto& next) -> v60::task<bool> {
        const auto is_matched = next.match(req.method(), req.path());
        if (!is_matched) {
            std::cerr << "404 [" << to_string(verb) << "] \"" << target << "\"\n";

            resp.status(404);
            co_await resp.send("Not found: " + std::string(req.path()));
            co_return false;
        }
        co_return co_await next(std::move(req), std::move(resp));
    };

    auto server_fault_mw = [&](v60::Request auto req,
                               v60::Response auto resp,
                               v60::Routable auto& next) -> v60::task<bool> {
        auto resp_bak = resp;
        try {
            co_return co_await next(std::move(req), std::move(resp));
        } catch (std::exception& err) {
            std::cerr << "500 [" << to_string(verb) << "] \"" << target
                      << "\": " << err.what() << '\n';
        }

        resp_bak.status(500);
        co_await resp_bak.send("Internal server error!");
        co_return false;
    };

    auto reqq = v60::request<v60::object<>, std::string_view>{
        v60::base_request{std::move(req)}, {}, body};
    auto root = v60::use(server_fault_mw, v60::use(not_found_mw, app()));

    http::response<http::string_body> res;
    res.set(http::field::server, "v60_over_" BOOST_BEAST_VERSION_STRING);
    res.version(reqq.version());
    res.keep_alive(req.keep_alive());

    v60::response<Send> resp(std::move(send), std::move(res));
    const auto route_res = root.match(reqq.method(), target) &&
                           co_await root(std::move(reqq), std::move(resp));

    if (route_res) {
        std::cerr << "200 [" << to_string(verb) << "] \"" << target << "\"\n";
    }
}

net::awaitable<void> do_session(beast::tcp_stream stream) {
    bool close = false;

    auto lambda =
        [&]<bool isRequest, class Body, class Fields>(
            http::message<isRequest, Body, Fields>&& msg) -> net::awaitable<void> {
        close = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        co_await http::async_write(stream, sr, net::use_awaitable);
        //        http::write(stream, sr);
    };

    beast::flat_buffer buffer;

    try {

        for (;;) {
            http::request<http::string_body> req;

            co_await http::async_read(stream, buffer, req, net::use_awaitable);

            co_await handle_request(std::move(req), std::move(lambda));
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


void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
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

int main() {
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(8080);
    auto const threads = std::max<int>(1, 4);

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    boost::asio::co_spawn(
        ioc, do_listen(ioc, tcp::endpoint{address, port}), boost::asio::detached);

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i) v.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    for (auto& t : v) {
        t.join();
    }
}