#include <iostream>
#include <v60/end_point.hpp>
#include <v60/group.hpp>
#include <v60/middleware.hpp>
#include <v60/server.hpp>

auto user_router() {
    using namespace v60;

    auto name_handler = [](Request auto req, Response auto resp) -> task<void> {
        std::cerr << "In correct handler\n";

        req.params.for_each_member([&]<auto key>(auto& val) {
            std::cerr << std::string_view(key) << ": " << val << '\n';
        });

        std::cerr << &resp << '\n';
        co_await resp.send("hello");
    };

    auto age_handler = [](Request auto req, Response auto resp) -> task<void> {
        co_await resp.send("age: 1234");

        //        std::cerr << req.body << '\n';
        req.body.for_each_member([&]<auto k>(auto& val) {
            std::cerr << std::string_view(k) << ": " << val << '\n';
        });
        req.params.for_each_member([&]<auto key>(auto& val) {
            std::cerr << std::string_view(key) << ": " << val << '\n';
        });
    };

    return group(get<"/name">(name_handler),
                 use(object_body<object<member<"age", int>>>, post<"/age">(age_handler)));
}

auto app() {
    using namespace v60;
    return bind<"/user/:userId">(user_router());
}

int main() {
    using namespace v60;
    v60::server s(use(server_fault_mw, use(not_found_mw, app())));
    s.listen(8080);
}