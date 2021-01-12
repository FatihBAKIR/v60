#pragma once

#include <v60/fixed_string.hpp>
#include <v60/internal/ctre.hpp>
#include <v60/object.hpp>

namespace meta {
template<size_t N>
constexpr v60::fixed_string<N + 1> to_ascii_str(const ctll::fixed_string<N>& str) {
    v60::fixed_string<N + 1> res{};
    std::transform(
        str.begin(), str.end(), res.begin(), [](auto val) { return char(val); });
    return res;
}

template<size_t N>
constexpr ctll::fixed_string<N - 1> to_ct_str(const v60::fixed_string<N>& str) {
    return ctll::fixed_string<N - 1>(str.val);
}

template<class T>
struct to_object_t;

template<class Iter, class... Names, size_t... Is>
struct to_object_t<ctre::regex_results<Iter, ctre::captured_content<Is, Names>...>> {
    using type = v60::object<v60::member<to_ascii_str(Names::name), std::string_view>...>;
};

template<class T>
using ObjectT = typename to_object_t<T>::type;
} // namespace meta
