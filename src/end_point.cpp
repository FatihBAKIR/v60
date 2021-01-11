#include <v60/end_point.hpp>

namespace v60 {
inline auto sample() {
    return get<"/foo">([](const Request auto&, const Response auto&) {});
}

static_assert(Routable<decltype(sample())>);
}