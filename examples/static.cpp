#include "v60/http.hpp"
#include "v60/request.hpp"
#include "v60/response.hpp"
#include <string>
#include <v60/v60.hpp>

struct static_route {
    bool match(v60::http::verb v, std::string_view path) const {
        return v == v60::http::verb::get;
    }

    v60::task<bool> operator()(v60::request<v60::object<>, std::string_view> req, v60::any_response resp) const {
        co_await resp.send("hello");
        co_return true;
    }
};

static_assert(v60::RoutableOf<static_route, v60::request<v60::object<>, std::string_view>>);

int main() {
    using namespace v60;

    server s(static_route{});
    s.listen(8080);
}