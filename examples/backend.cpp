#include <iostream>
#include <v60/v60.hpp>

auto user_router() {
    using namespace v60;

    auto name_handler = [](Request auto req, Response auto resp) -> task<void> {
        co_await resp.send("hello " + std::string(get<"userId">(req.params)));
    };

    auto age_handler = [](Request auto req, Response auto resp) -> task<void> {
        co_await resp.json(42);
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
    v60::server s(use(profile_mw, use(server_fault_mw, use(not_found_mw, app()))));
    s.listen(8080);
}