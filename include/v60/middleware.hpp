#pragma once

#include <v60/request.hpp>
#include <v60/routing.hpp>

namespace v60 {
template<class R, class N, class T>
concept Middleware = requires(T t) {
    { static_cast<const T&>(t)(std::declval<R&>(), std::declval<N&>()) }
    ->meta::awaitable;
};

template<Routable NextT, class FnT>
struct middleware {
    NextT m_next;
    FnT m_fn;

    bool match(http::verb v, std::string_view path) const {
        return m_next.match(v, path);
    }

    template<Request Req, Response Resp>
    task<bool> operator()(Req&& req, Resp&& resp) const {
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
    if (req.method() != http::verb::post) {
        return next(std::forward<decltype(req)>(req), std::forward<decltype(resp)>(resp));
    }

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
} // namespace v60