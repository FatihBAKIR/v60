#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <v60/async.hpp>
#include <v60/binding.hpp>
#include <v60/ctre.hpp>
#include <v60/meta.hpp>
#include <v60/object.hpp>
#include <v60/routing.hpp>

namespace v60 {
template<http::verb method, class FnT>
class end_point {
public:
    FnT m_fn;

    bool match(http::verb v, std::string_view path) const {
        return v == method && path.empty();
    }

    template<Request ReqT, Response Resp>
    task<bool> operator()(ReqT req, Resp resp) const {
        static constexpr auto is_coroutine =
            meta::awaitable<decltype(m_fn(std::move(req), std::move(resp)))>;

        if constexpr (is_coroutine) {
            co_await m_fn(std::move(req), std::move(resp));
        } else {
            m_fn(std::move(req), std::move(resp));
        }

        co_return true;
    }
};

template <class FnT>
auto get(FnT&& fn) {
    return end_point<http::verb::get, FnT>{std::forward<FnT>(fn)};
}

template <class FnT>
auto post(FnT&& fn) {
    return end_point<http::verb::post, FnT>{std::forward<FnT>(fn)};
}

template<fixed_string Path, class FnT>
auto get(FnT&& fn) {
    return bind<Path>(get(std::forward<FnT>(fn)));
}

template<fixed_string Path, class FnT>
auto post(FnT&& fn) {
    return bind<Path>(post(std::forward<FnT>(fn)));
}
} // namespace v60