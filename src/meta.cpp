#include <v60/meta.hpp>

namespace v60::meta {
namespace {
struct test_less {
    constexpr bool operator()(meta::id<char>, meta::id<float>) {
        return false;
    }
    constexpr bool operator()(meta::id<float>, meta::id<int>) {
        return false;
    }
    constexpr bool operator()(meta::id<char>, meta::id<int>) {
        return false;
    }
    constexpr bool operator()(meta::id<float>, meta::id<char>) {
        return true;
    }
    constexpr bool operator()(meta::id<int>, meta::id<float>) {
        return true;
    }
    constexpr bool operator()(meta::id<int>, meta::id<char>) {
        return true;
    }
};

using l = tlist<float, char, int>;

static_assert(std::is_same_v<sort_list_t<l, test_less>, tlist<int, float, char>>);
} // namespace
} // namespace v60::meta