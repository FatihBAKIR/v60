#pragma once

#include <cstdint>
#include <type_traits>
#include <v60/async.hpp>

namespace v60::meta {
template<class T>
struct id {
    using type = T;
};

template<auto...>
class list;

template<class...>
class tlist;

template<class Comparator>
struct eq {
    template<class A, class B>
    constexpr bool operator()(A a, B b) const {
        return !Comparator{}(a, b) && !Comparator{}(b, a);
    }
};

template<class A, class B>
struct concat;

template<auto... Ts, auto... Us>
struct concat<list<Ts...>, list<Us...>> {
    using type = list<Ts..., Us...>;
};

template<class... Ts, class... Us>
struct concat<tlist<Ts...>, tlist<Us...>> {
    using type = tlist<Ts..., Us...>;
};

template<class... Ts>
using concat_t = typename concat<Ts...>::type;

template<int Count, class... Ts>
struct take;

template<int Count, class... Ts>
using take_t = typename take<Count, Ts...>::type;

template<auto... Ts>
struct take<0, list<Ts...>> {
    using type = list<>;
    using rest = list<Ts...>;
};

template<class... Ts>
struct take<0, tlist<Ts...>> {
    using type = tlist<>;
    using rest = tlist<Ts...>;
};

template<auto A, auto... Ts>
struct take<1, list<A, Ts...>> {
    using type = list<A>;
    using rest = list<Ts...>;
};

template<class A, class... Ts>
struct take<1, tlist<A, Ts...>> {
    using type = tlist<A>;
    using rest = tlist<Ts...>;
};

template<int Count, auto A, auto... Ts>
struct take<Count, list<A, Ts...>> {
    using type = concat_t<list<A>, take_t<Count - 1, list<Ts...>>>;
    using rest = typename take<Count - 1, list<Ts...>>::rest;
};

template<int Count, class A, class... Ts>
struct take<Count, tlist<A, Ts...>> {
    using type = concat_t<tlist<A>, take_t<Count - 1, tlist<Ts...>>>;
    using rest = typename take<Count - 1, tlist<Ts...>>::rest;
};

template<class Left, class Right, class Comparator>
struct merge;

template<class... Ts>
using merge_t = typename merge<Ts...>::type;

template<auto... Bs, class Comparator>
struct merge<list<>, list<Bs...>, Comparator> {
    using type = list<Bs...>;
};

template<class... Bs, class Comparator>
struct merge<tlist<>, tlist<Bs...>, Comparator> {
    using type = tlist<Bs...>;
};

template<auto... As, class Comparator>
struct merge<list<As...>, list<>, Comparator> {
    using type = list<As...>;
};

template<class... As, class Comparator>
struct merge<tlist<As...>, tlist<>, Comparator> {
    using type = tlist<As...>;
};

template<auto AHead, auto... As, auto BHead, auto... Bs, class Comparator>
struct merge<list<AHead, As...>, list<BHead, Bs...>, Comparator> {
    using type = std::conditional_t<
        Comparator{}(AHead, BHead),
        concat_t<list<AHead>, merge_t<list<As...>, list<BHead, Bs...>, Comparator>>,
        concat_t<list<BHead>, merge_t<list<AHead, As...>, list<Bs...>, Comparator>>>;
};

template<class AHead, class... As, class BHead, class... Bs, class Comparator>
struct merge<tlist<AHead, As...>, tlist<BHead, Bs...>, Comparator> {
    using type = std::conditional_t<
        Comparator{}(id<AHead>{}, id<BHead>{}),
        concat_t<tlist<AHead>, merge_t<tlist<As...>, tlist<BHead, Bs...>, Comparator>>,
        concat_t<tlist<BHead>, merge_t<tlist<AHead, As...>, tlist<Bs...>, Comparator>>>;
};

template<class List, class Comparator>
struct sort_list;

template<class... Ts>
using sort_list_t = typename sort_list<Ts...>::type;

template<class Comparator>
struct sort_list<list<>, Comparator> {
    using type = list<>;
};

template<class Comparator>
struct sort_list<tlist<>, Comparator> {
    using type = tlist<>;
};

template<auto A, class Comparator>
struct sort_list<list<A>, Comparator> {
    using type = list<A>;
};

template<class A, class Comparator>
struct sort_list<tlist<A>, Comparator> {
    using type = tlist<A>;
};

template<auto A, auto B, class Comparator>
struct sort_list<list<A, B>, Comparator> {
    using type = std::conditional_t<Comparator{}(A, B), list<A, B>, list<B, A>>;
};

template<class A, class B, class Comparator>
struct sort_list<tlist<A, B>, Comparator> {
    using type =
        std::conditional_t<Comparator{}(id<A>{}, id<B>{}), tlist<A, B>, tlist<B, A>>;
};

template<auto... Types, class Comparator>
struct sort_list<list<Types...>, Comparator> {
    static constexpr auto first_size = sizeof...(Types) / 2;
    using split = take<first_size, list<Types...>>;
    using type = merge_t<sort_list_t<typename split::type, Comparator>,
                         sort_list_t<typename split::rest, Comparator>,
                         Comparator>;
};

template<class... Types, class Comparator>
struct sort_list<tlist<Types...>, Comparator> {
    static constexpr auto first_size = sizeof...(Types) / 2;
    using split = take<first_size, tlist<Types...>>;
    using type = merge_t<sort_list_t<typename split::type, Comparator>,
                         sort_list_t<typename split::rest, Comparator>,
                         Comparator>;
};

template<template<class...> class Ins, class...>
struct instantiate;

template<template<class...> class Ins, class... Ts>
struct instantiate<Ins, tlist<Ts...>> {
    using type = Ins<Ts...>;
};

template<template<class...> class Ins, class... Ts>
using instantiate_t = typename instantiate<Ins, Ts...>::type;

template<class A, class B, class Comparator>
struct union_;

template<class... Ts>
using union_t = typename union_<Ts...>::type;

template<class Comparator>
struct union_<tlist<>, tlist<>, Comparator> {
    using type = tlist<>;
};

template<class... As, class Comparator>
struct union_<tlist<As...>, tlist<>, Comparator> {
    using type = tlist<As...>;
};

template<class... As, class Comparator>
struct union_<tlist<>, tlist<As...>, Comparator> {
    using type = tlist<As...>;
};

template<class AHead, class... As, class BHead, class... Bs, class Comparator>
struct union_<tlist<AHead, As...>, tlist<BHead, Bs...>, Comparator> {
    static constexpr auto head_eq = eq<Comparator>{}(id<AHead>{}, id<BHead>{});
    static constexpr auto a_lt = Comparator{}(id<AHead>{}, id<BHead>{});

    /**
     * If the first elements of both sorted lists are equal, then it's in the
     * intersection. Otherwise, we drop the smaller of the heads and continue
     * intersecting, since there's no way it's in the other one.
     */
    using type = std::conditional_t<
        head_eq,
        concat_t<tlist<AHead>, union_t<tlist<As...>, tlist<Bs...>, Comparator>>,
        std::conditional_t<
            a_lt,
            concat_t<tlist<AHead>,
                     union_t<tlist<As...>, tlist<BHead, Bs...>, Comparator>>,
            concat_t<tlist<BHead>,
                     union_t<tlist<AHead, As...>, tlist<Bs...>, Comparator>>>>;
};

template<class A, class B, class Comparator>
struct intersection;

template<class... Ts>
using intersection_t = typename intersection<Ts...>::type;

template<class Comparator>
struct intersection<tlist<>, tlist<>, Comparator> {
    using type = tlist<>;
};

template<class... As, class Comparator>
struct intersection<tlist<As...>, tlist<>, Comparator> {
    using type = tlist<>;
};

template<class... As, class Comparator>
struct intersection<tlist<>, tlist<As...>, Comparator> {
    using type = tlist<>;
};

template<auto... As, class Comparator>
struct intersection<list<As...>, list<>, Comparator> {
    using type = list<>;
};

template<auto... As, class Comparator>
struct intersection<list<>, list<As...>, Comparator> {
    using type = list<>;
};

template<class AHead, class... As, class BHead, class... Bs, class Comparator>
struct intersection<tlist<AHead, As...>, tlist<BHead, Bs...>, Comparator> {
    static constexpr auto head_eq = eq<Comparator>{}(id<AHead>{}, id<BHead>{});
    static constexpr auto a_lt = Comparator{}(id<AHead>{}, id<BHead>{});

    /**
     * If the first elements of both sorted lists are equal, then it's in the
     * intersection. Otherwise, we drop the smaller of the heads and continue
     * intersecting, since there's no way it's in the other one.
     */
    using type = std::conditional_t<
        head_eq,
        concat_t<tlist<AHead>, intersection_t<tlist<As...>, tlist<Bs...>, Comparator>>,
        std::conditional_t<
            a_lt,
            intersection_t<tlist<As...>, tlist<BHead, Bs...>, Comparator>,
            intersection_t<tlist<AHead, As...>, tlist<Bs...>, Comparator>>>;
};

template<class A, class B, class Comparator>
struct difference;

template<class... Ts>
using difference_t = typename difference<Ts...>::type;

template<class Comparator>
struct difference<tlist<>, tlist<>, Comparator> {
    using type = tlist<>;
};

template<class... As, class Comparator>
struct difference<tlist<As...>, tlist<>, Comparator> {
    using type = tlist<As...>;
};

template<class... As, class Comparator>
struct difference<tlist<>, tlist<As...>, Comparator> {
    using type = tlist<As...>;
};

template<class AHead, class... As, class BHead, class... Bs, class Comparator>
struct difference<tlist<AHead, As...>, tlist<BHead, Bs...>, Comparator> {
    static constexpr auto head_eq = eq<Comparator>{}(id<AHead>{}, id<BHead>{});
    static constexpr auto a_lt = Comparator{}(id<AHead>{}, id<BHead>{});

    /**
     * If the first elements of both sorted lists are equal, then it's in the
     * intersection. Otherwise, we drop the smaller of the heads and continue
     * intersecting, since there's no way it's in the other one.
     */
    using type = std::conditional_t<
        head_eq,
        difference_t<tlist<As...>, tlist<Bs...>, Comparator>,
        std::conditional_t<
            a_lt,
            concat_t<tlist<AHead>,
                     difference_t<tlist<As...>, tlist<BHead, Bs...>, Comparator>>,
            concat_t<tlist<BHead>,
                     difference_t<tlist<AHead, As...>, tlist<Bs...>, Comparator>>>>;
};

namespace detail {
template<auto needle, auto... haystack, size_t... Is>
constexpr int32_t do_index_of(std::index_sequence<Is...>) {
    int32_t res = 0;
    ((haystack == needle && (res = (Is + 1))) || ...);
    return res - 1;
}
} // namespace detail

template<auto needle, auto... haystack>
constexpr int32_t index_of() {
    return detail::do_index_of<needle, haystack...>(
        std::make_index_sequence<sizeof...(haystack)>{});
}

template<class, template<class...> class>
struct is_instance : public std::false_type {};

template<class... Ts, template<class...> class U>
struct is_instance<U<Ts...>, U> : public std::true_type {};

namespace detail {
auto do_await = [](auto& prom) -> v60::task<void> { co_await prom; };
}

template<class T>
concept awaitable = requires(T t) {
    detail::do_await(t);
};
} // namespace v60::meta
