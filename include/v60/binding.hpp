#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <v60/async.hpp>
#include <v60/ctre.hpp>
#include <v60/meta.hpp>
#include <v60/object.hpp>
#include <v60/routing.hpp>

namespace v60 {
template<fixed_string Path, Routable NextT>
class binding {
public:
    NextT m_next;

    static constexpr auto re() {
        return ctre::re<::meta::to_ct_str(detail::path_pattern_to_regex<Path>())>();
    }

    using RE = decltype(re());
    using REResT = decltype(RE::starts_with(""));
    using ParamsObjectT = ::meta::ObjectT<REResT>;

    bool match(http::verb v, std::string_view path) const {
        auto m = RE::starts_with(path);

        if (!m) {
            return false;
        }

        auto all_match = m.template get<0>();

        auto dist = std::distance(path.begin(), all_match.end());
        auto remain = path.substr(dist);

        auto res = m_next.match(v, remain);
        return res;
    }

    template<Request Req, Response Resp>
    task<bool> operator()(Req req, Resp resp) const {
        auto m = RE::starts_with(req.remaining());

        assert(m);

        auto all_match = m.template get<0>();
        auto len = std::distance(req.remaining().begin(), all_match.end());
        assert(m_next.match(req.method(), req.remaining().substr(len)));

        using CombinedParams = object_and<typename Req::params_type, ParamsObjectT>;

        CombinedParams params = std::move(req.params);
        ParamsObjectT::for_each_key([&]<auto key>() {
            auto val = std::string_view(m.template get_by_name<::meta::to_ct_str(key)>());
            params.template get<key>() = val;
        });

        req.consume(static_cast<int>(len));

        return m_next(std::move(req).with_params(std::move(params)), std::move(resp));
    }
};

template<fixed_string Path, Routable NextT>
auto bind(NextT&& next) {
    return binding<Path, NextT>{std::move(next)};
}
} // namespace v60
