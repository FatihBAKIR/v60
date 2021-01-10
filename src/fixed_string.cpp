#include <v60/fixed_string.hpp>

namespace v60 {
static_assert(fixed_string("foo").size() == 4);

namespace {
template<fixed_string str>
class x {
};

x<"foo"> x_;
} // namespace
} // namespace v60