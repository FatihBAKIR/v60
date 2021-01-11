#pragma once

#include <algorithm>
#include <ctre.hpp>
#include <optional>
#include <string_view>
#include <v60/fixed_string.hpp>
#include <v60/meta.hpp>
#include <v60/request.hpp>
#include <v60/response.hpp>

namespace v60 {
namespace detail {
template<class RangeT>
constexpr auto dist(RangeT& rng) {
    int res = 0;
    for (auto& _ : rng) {
        ++res;
    }
    return res;
}

template<fixed_string pattern>
constexpr auto path_pattern_to_regex() {
    constexpr auto pattern_sv = std::string_view(pattern);
    constexpr auto sres = ctre::range<":(\\w+)">(pattern_sv);

    constexpr int size = detail::dist(sres);

    if constexpr (size == 0) {
        return pattern;
    }

    fixed_string<pattern.size() + size * 7> res{};

    auto res_it = res.begin();
    auto beg = pattern_sv.begin();
    int off = 0;
    for (auto& m : sres) {
        auto len = std::distance(beg, m.get<0>().begin());
        res_it = std::copy_n(beg, len, res_it);
        beg += len;

        res_it = std::copy_n("(?<", 3, res_it);
        res_it = std::copy(m.get<1>().begin(), m.get<1>().end(), res_it);
        res_it = std::copy_n(">\\w+)", 5, res_it);

        beg += std::distance(m.get<0>().begin(), m.get<0>().end());
    }

    return res;
}

auto dummy_send = [](auto&&) -> task<void> {};
} // namespace detail

template<class ParamT, class BodyT, class T>
concept RoutableOf = requires(T t) {
    {
        static_cast<const T&>(t).match(std::declval<http::verb>(),
                                       std::declval<std::string_view>())
    }
    ->meta::convertible_to<bool>;

    {
        static_cast<const T&>(t)(std::declval<request<ParamT, BodyT>&&>(),
                                 std::declval<response<decltype(detail::dummy_send)>&&>())
    }
    ->meta::awaitable;
};

template<class T>
concept Routable = RoutableOf<object<>, object<>, T>;

namespace detail {
class null_end_point {
public:
    bool match(http::verb, std::string_view) const {
        return false;
    }

    template<Request R, Response Resp>
    task<bool> operator()(R, Resp) const {
        co_return false;
    }
};

static_assert(Routable<null_end_point>);

template<Request Req, Response Resp>
struct virt_routable {
    virtual bool match(http::verb, std::string_view) const = 0;
    virtual task<bool> operator()(Req req, Resp resp) const = 0;
    virtual ~virt_routable() = default;
};

template<Routable Rt, Request Req, Response Resp>
struct erased_routable : virt_routable<Req, Resp> {
    erased_routable(Rt rt)
        : m_rt{std::move(rt)} {
    }

    bool match(http::verb verb, std::string_view view) const override {
        return m_rt.match(verb, view);
    }

    task<bool> operator()(Req req, Resp resp) const override {
        return m_rt(std::move(req), std::move(resp));
    }

    Rt m_rt;
};
} // namespace detail

template<Request Req, Response Resp>
struct any_routable {
    template<Routable Rt>
    any_routable(Rt&& rt)
        : m_rt{std::make_unique<detail::erased_routable<Rt, Req, Resp>>(
              std::forward<Rt>(rt))} {
    }

    bool match(http::verb verb, std::string_view view) const {
        return m_rt->match(verb, view);
    }

    task<bool> operator()(Req req, Resp resp) const {
        return (*m_rt)(std::move(req), std::move(resp));
    }

private:
    std::unique_ptr<detail::virt_routable<Req, Resp>> m_rt;
};
} // namespace v60
