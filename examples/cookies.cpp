#include <v60/v60.hpp>

int main() {
    using namespace v60;

    server s(use(cookie_parser<object<member<"name", std::string>>>,
                 get<"/hello">([](Request auto req, Response auto resp) -> task<void> {
                     co_await resp.send("Hello from v60, " + get<"name">(req.cookies));
                 })));
    s.listen(8080);
}