#pragma once

#include <string_view>
#include <v60/async.hpp>
#include <v60/http.hpp>
#include <v60/object.hpp>

namespace v60 {
template<class SenderT>
class response {
public:
    explicit response(SenderT&& sender, http::str_response&& resp = {})
        : m_send{std::move(sender)}
        , m_resp{std::move(resp)} {
        m_resp.result(200);
    }

    void status(int code) {
        m_resp.result(code);
    }

    template<Object ResObj>
    task<void> json(const ResObj& obj);

    task<void> json(const nlohmann::json& data) {
        m_resp.body() = std::string(data);
        m_resp.content_length(m_resp.body().size());
        set_content_type("application/json");

        co_await m_send(std::move(m_resp));
    }

    task<void> send(std::string_view data) {
        m_resp.body() = std::string(data);
        m_resp.content_length(m_resp.body().size());

        co_await m_send(std::move(m_resp));
    }

    void set_content_type(std::string_view type) {
        set_header("content-type", type);
    }

    void set_header(std::string_view key, std::string_view value) {
        m_resp.set(boost::string_view(key.data(), key.size()), boost::string_view(value.data(), value.size()));
    }

private:
    SenderT m_send;
    http::str_response m_resp;
};

template<class T>
concept Response = meta::is_instance<T, response>::value;
} // namespace v60
