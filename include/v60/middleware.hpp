#pragma once

#include <v60/internal/simdjson.h>
#include <v60/meta.hpp>
#include <v60/request.hpp>
#include <v60/routing.hpp>

namespace v60 {
template<Routable NextT, class FnT>
struct middleware {
    NextT m_next;
    FnT m_fn;

    bool match(http::verb v, std::string_view path) const {
        return m_next.match(v, path);
    }

    template<Request Req, Response Resp>
    task<bool> operator()(Req req, Resp resp) const {
        static constexpr auto is_coroutine =
            meta::awaitable<decltype(m_fn(std::move(req), std::move(resp), m_next))>;

        if constexpr (is_coroutine) {
            co_return co_await m_fn(std::move(req), std::move(resp), m_next);
        } else {
            co_return m_fn(std::move(req), std::move(resp), m_next);
        }
    }
};

template<class FnT, Routable N>
auto use(FnT&& m, N&& next) {
    return middleware<N, FnT>{std::forward<N>(next), std::forward<FnT>(m)};
}

template<Object ObjT>
auto object_body = [](Request auto req, Response auto resp, Routable auto& next) {
    using namespace simdjson;
    using namespace simdjson::builtin; // for ondemand
    ondemand::parser parser;
    ondemand::document elems = parser.iterate(req.body);

    ObjT body;
    body.for_each_member([&]<auto key>(auto& mem) {
        mem = elems[std::string_view(key)]; //.get<decltype(mem)>();
    });

    return next(std::move(req).with_body(std::move(body)), std::move(resp));
};


inline constexpr auto logging_mw =
    [](Request auto req, Response auto resp, Routable auto& next) -> task<bool> {
    std::cerr << req.method() << " \"" << req.path() << "\"\n";
    return next(std::move(req), std::move(resp));
};

inline constexpr auto profile_mw =
    [](Request auto req, Response auto resp, Routable auto& next) -> task<bool> {
    auto begin = std::chrono::high_resolution_clock::now();
    auto res = co_await next(std::move(req), std::move(resp));
    auto end = std::chrono::high_resolution_clock::now();
    std::cerr
        << "["
        << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()
        << "us]\n";

    co_return res;
};

inline constexpr auto not_found_mw =
    [](Request auto req, Response auto resp, Routable auto& next) -> task<bool> {
    const auto is_matched = next.match(req.method(), req.path());
    if (!is_matched) {
        std::cerr << "404 [" << to_string(req.method()) << "] \"" << req.path() << "\"\n";

        resp.status(404);
        co_await resp.send("Not found: " + std::string(req.path()));
        co_return false;
    }
    co_return co_await next(std::move(req), std::move(resp));
};

inline constexpr auto server_fault_mw =
    [](Request auto req, Response auto resp, Routable auto& next) -> task<bool> {
    auto method = req.method();
    auto path = req.path();
    auto resp_bak = resp;
    try {
        co_return co_await next(std::move(req), std::move(resp));
    } catch (std::exception& err) {
        std::cerr << "500 [" << to_string(method) << "] \"" << path
                  << "\": " << err.what() << '\n';
    }
    resp_bak.status(500);
    co_await resp_bak.send("Internal server error!");
    co_return false;
};

inline std::vector<std::string_view> split(std::string_view str,
                                           std::string_view delims = " ") {
    std::vector<std::string_view> output;
    output.reserve(str.size() / 2);

    for (auto first = str.data(), second = str.data(), last = first + str.size();
         second != last && first != last;
         first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

        if (first != second)
            output.emplace_back(first, second - first);
    }

    return output;
}

template<Object Cookies>
struct cookie_mixin {
    Cookies cookies;
};

template<Object Cookies>
auto cookie_parser =
    [](Request auto req, Response auto resp, Routable auto& next) -> task<bool> {
    auto cookies_header = req.header("Cookie");
    if (!cookies_header) {
        co_return false;
    }

    std::cerr << "Received cookies: " << *cookies_header << '\n';

    auto parts = split(*cookies_header, "; ");

    std::map<std::string_view, std::string_view> kv;
    for (auto part : parts) {
        if (auto [whole, key, val] = ctre::match<"(\\S+)=(\\S+)">(part); whole) {
            kv.emplace(std::string_view(key), std::string_view(val));
        }
    }

    cookie_mixin<Cookies> mixin;
    mixin.cookies.for_each_member([&]<auto key>(auto& mem) {
        mem = kv[std::string_view(key)];
    });

    co_return co_await next(std::move(req).with_mixin(std::move(mixin)), std::move(resp));
};
} // namespace v60
