#include <v60/object.hpp>

namespace v60 {
static_assert(Object<object<>>);
static_assert(Object<object<member<"foo", int>>>);
}