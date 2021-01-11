#pragma once

#include <optional>
#include <string_view>
#include <v60/http.hpp>
#include <v60/meta.hpp>
#include <v60/object.hpp>

namespace v60 {
class base_request {
public:
    explicit base_request(http::str_request&& raw)
        : m_raw{std::move(raw)} {
        m_remaining = path();
    }

    std::string_view path() const {
        return std::string_view(m_raw.target().data(), m_raw.target().size());
    }

    std::string_view remaining() const {
        return m_remaining;
    }

    void consume(int n) {
        m_remaining = m_remaining.substr(n);
    }

    unsigned version() const {
        return m_raw.version();
    }

    http::verb method() const {
        return m_raw.method();
    }

    std::optional<std::string_view> header(std::string_view key) const {
        auto it = m_raw.find(boost::string_view(key.data(), key.size()));
        if (it == m_raw.end()) {
            return {};
        }
        auto val = it->value();
        return std::string_view(val.data(), val.size());
    }

private:
    std::string_view m_remaining;
    http::str_request m_raw;
};

template<Object Params = object<>, class Body = object<>, class... Mixins>
class request
    : public base_request
    , public Mixins... {
public:
    using body_type = Body;
    using params_type = Params;

    Params params;
    Body body;

    template<Object NewParams>
    request<NewParams, Body, Mixins...> with_params(NewParams&& new_params) & {
        return request<NewParams, Body, Mixins...>{{static_cast<base_request&>(*this)},
                                                   {static_cast<Mixins&>(*this)}...,
                                                   std::forward<NewParams>(new_params),
                                                   body};
    }

    template<Object NewParams>
    request<NewParams, Body, Mixins...> with_params(NewParams&& new_params) && {
        return request<NewParams, Body, Mixins...>{
            {std::move(static_cast<base_request&>(*this))},
            {std::move(static_cast<Mixins&>(*this))}...,
            std::forward<NewParams>(new_params),
            std::move(body)};
    }

    template<class NewBody>
    request<Params, NewBody, Mixins...> with_body(NewBody&& new_body) & {
        return request<Params, NewBody, Mixins...>{{static_cast<base_request&>(*this)},
                                                   {static_cast<Mixins&>(*this)}...,
                                                   params,
                                                   std::forward<NewBody>(new_body)};
    }

    template<class NewBody>
    request<Params, NewBody, Mixins...> with_body(NewBody&& new_body) && {
        return request<Params, NewBody, Mixins...>{
            {std::move(static_cast<base_request&>(*this))},
            {std::move(static_cast<Mixins&>(*this))}...,
            std::move(params),
            std::forward<NewBody>(new_body)};
    }
};

template<class T>
concept Request = meta::is_instance<T, request>::value&& requires(T t) {
    {T::params_type};
    {t.params};
};

template<class Body, class T>
concept RequestWith = Request<T>&& std::is_same_v<typename T::body_type, Body>;
} // namespace v60