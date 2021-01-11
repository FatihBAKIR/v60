#pragma once

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

auto json_body = []<Object Params, std::convertible_to<std::string_view> Body>(
                     request<Params, Body> req, Response auto resp, Routable auto& next) {
    return next(
        std::forward<decltype(req)>(req).with_body(nlohmann::json::parse(req.body)),
        std::move(resp));
};

template<Object ObjT>
auto object_body =
    []<Object Params, std::convertible_to<std::string_view> Body>(
        request<Params, Body> req, Response auto resp, Routable auto& next) {
        auto json = nlohmann::json::parse(req.body);

        ObjT body;
        body.for_each_member([&]<auto key>(auto& mem) {
            mem = json[std::string(std::string_view(key))];
        });

        return next(std::move(req).with_body(std::move(body)), std::move(resp));
    };


auto profile_mw = [](v60::Request auto req,
                     v60::Response auto resp,
                     v60::Routable auto& next) -> v60::task<bool> {
    auto begin = std::chrono::high_resolution_clock::now();
    auto res = co_await next(std::move(req), std::move(resp));
    auto end = std::chrono::high_resolution_clock::now();
    std::cerr
        << "["
        << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()
        << "us]\n";

    co_return res;
};

auto not_found_mw = [](v60::Request auto req,
                       v60::Response auto resp,
                       v60::Routable auto& next) -> v60::task<bool> {
    const auto is_matched = next.match(req.method(), req.path());
    if (!is_matched) {
        std::cerr << "404 [" << to_string(req.method()) << "] \"" << req.path() << "\"\n";

        resp.status(404);
        co_await resp.send("Not found: " + std::string(req.path()));
        co_return false;
    }
    co_return co_await next(std::move(req), std::move(resp));
};

auto server_fault_mw = [](v60::Request auto req,
                          v60::Response auto resp,
                          v60::Routable auto& next) -> v60::task<bool> {
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
} // namespace v60