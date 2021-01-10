#pragma once

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
        co_return(
            (std::get<Routes>(m_routes).match(req.method(), req.remaining()) &&
             co_await std::get<Routes>(m_routes)(std::move(req), std::move(resp))) ||
            ...);
    }

private:
    std::tuple<Routes...> m_routes;
};
} // namespace v60