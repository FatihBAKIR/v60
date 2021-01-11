#include <v60/routing.hpp>

namespace v60 {
namespace {
class null_end_point {
public:
    bool match(http::verb, std::string_view) const {
        return false;
    }

    template<Request R, Response Resp>
    task<bool> operator()(R, Resp) const {
        co_return false;
    }
};

static_assert(Routable<null_end_point>);
} // namespace
} // namespace v60