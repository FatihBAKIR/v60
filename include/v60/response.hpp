#pragma once

#include <boost/json.hpp>
#include <string_view>
#include <v60/async.hpp>
#include <v60/http.hpp>
#include <v60/internal/simdjson.h>
#include <v60/object.hpp>

namespace v60 {
template<class SenderT>
class response {
public:
    explicit response(SenderT&& sender, http::str_response&& resp = {})
        : m_send{std::move(sender)}
        , m_resp{std::move(resp)} {
        m_resp.result(200);
        content_type("text/html");
    }

    void status(int code) {
        m_resp.result(code);
    }

    template<Object ResObj>
    task<void> json(const ResObj& obj);

    task<void> json(int64_t data) {
        content_type("application/json");
        return send(std::to_string(data));
    }

    task<void> json(const boost::json::value& data) {
        content_type("application/json");
        return send(serialize(data));
    }

    task<void> send(std::string data) {
        m_resp.body() = std::move(data);
        m_resp.content_length(m_resp.body().size());

        return m_send(std::move(m_resp));
    }

    void content_type(std::string_view type) {
        header("content-type", type);
    }

    void header(std::string_view key, std::string_view value) {
        m_resp.set(boost::string_view(key.data(), key.size()),
                   boost::string_view(value.data(), value.size()));
    }

private:
    SenderT m_send;
    http::str_response m_resp;
};

template<class T>
concept Response = meta::is_instance<T, response>::value;

using any_send = std::function<task<void>(http::str_response)>;
using any_response = response<any_send>;
} // namespace v60
