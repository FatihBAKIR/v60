#pragma once

#include <functional>
#include <v60/async.hpp>
#include <v60/request.hpp>
#include <v60/response.hpp>

namespace v60 {
struct server_impl;
class server {
public:
    server(std::function<task<bool>(request<object<>, std::string_view>, any_response)>
               root_route);

    ~server();

    void listen(int port);

private:
    std::unique_ptr<server_impl> m_impl;
};
} // namespace v60