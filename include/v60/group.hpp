#pragma once

#include <utility>
#include <v60/async.hpp>
#include <v60/routing.hpp>

namespace v60 {
template<Routable... Routes>
class group {
public:
    group(Routes&&... rs)
        : m_routes(std::move(rs)...) {
    }

    bool match(http::verb v, std::string_view path) const {
        return (std::get<Routes>(m_routes).match(v, path) || ...);
    }

    template<Request ReqT, Response Resp>
    task<bool> operator()(ReqT req, Resp resp) const {
        auto doidx = [&]<std::size_t I>() -> task<bool> {
            if (!std::get<I>(m_routes).match(req.method(), req.remaining())) {
                co_return false;
            }
            co_return co_await std::get<I>(m_routes)(std::move(req), std::move(resp));
        };

        co_return co_await[&]<std::size_t... Is>(std::index_sequence<Is...>)->task<bool> {
            co_return((co_await doidx.template operator()<Is>()) || ...);
        }
        (std::make_index_sequence<sizeof...(Routes)>{});
    }

private:
    std::tuple<Routes...> m_routes;
};
} // namespace v60