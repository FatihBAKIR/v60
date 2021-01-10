#pragma once

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

private:
    std::string_view m_remaining;
    http::str_request m_raw;
};

template<Object Params = object<>, class Body = object<>>
class request : public base_request {
public:
    using body_type = Body;

    Params params;
    Body body;

    template<Object NewParams>
    request<NewParams, Body> with_params(NewParams&& new_params) & {
        return request<NewParams, Body>{{static_cast<base_request&>(*this)},
                                        std::forward<NewParams>(new_params),
                                        body};
    }

    template<Object NewParams>
    request<NewParams, Body> with_params(NewParams&& new_params) && {
        return request<NewParams, Body>{{std::move(static_cast<base_request&>(*this))},
                                        std::forward<NewParams>(new_params),
                                        std::move(body)};
    }

    template<class NewBody>
    request<Params, NewBody> with_body(NewBody&& new_body) & {
        return request<Params, NewBody>{
            {static_cast<base_request&>(*this)}, params, std::forward<NewBody>(new_body)};
    }

    template<class NewBody>
    request<Params, NewBody> with_body(NewBody&& new_body) && {
        return request<Params, NewBody>{{std::move(static_cast<base_request&>(*this))},
                                        std::move(params),
                                        std::forward<NewBody>(new_body)};
    }
};

template<class T>
concept Request = meta::is_instance<T, request>::value&& requires(T t) {
    {t.params};
};

template<class Body, class T>
concept RequestWith = Request<T>&& std::is_same_v<typename T::body_type, Body>;
} // namespace v60