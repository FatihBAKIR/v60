#include <v60/v60.hpp>

int main() {
    using namespace v60;

    server s(get<"/hello/:name">([](Request auto req, Response auto resp) -> task<void> {
        co_await resp.send("Hello from v60, " + std::string(get<"name">(req.params)));
    }));
    s.listen(8080);
}