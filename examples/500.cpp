#include <v60/v60.hpp>

int main() {
    using namespace v60;

    server s(
        use(server_fault_mw,
            get<"/hello/:name">([](Request auto req, Response auto resp) -> task<void> {
              co_await resp.send("Hello from v60, " +
                                 std::string(req.params.get<"name">()));
            })));
    s.listen(8080);
}