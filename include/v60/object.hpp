#pragma once

#include <iostream>
#include <json.hpp>
#include <tuple>
#include <v60/fixed_string.hpp>
#include <v60/meta.hpp>

namespace v60 {
template<fixed_string key, class T>
struct member {};

struct member_less {
    template<fixed_string left_key, class leftT, fixed_string right_key, class rightT>
    constexpr bool operator()(meta::id<member<left_key, leftT>>,
                              meta::id<member<right_key, rightT>>) {
        return std::less<>{}(std::string_view(left_key), std::string_view(right_key));
    }
};

template<class...>
struct object_impl;

template<class... Ts>
using object =
    meta::instantiate_t<object_impl, meta::sort_list_t<meta::tlist<Ts...>, member_less>>;

template<fixed_string... keys, class... Ts>
struct object_impl<member<keys, Ts>...> {
    using member_list = meta::tlist<member<keys, Ts>...>;

    object_impl() = default;

    object_impl(const object_impl&) = default;

    object_impl(object_impl&&) = default;

    template<fixed_string... OtherKeys, class... OtherTs>
    object_impl(const object_impl<member<OtherKeys, OtherTs>...>& rhs) {
        using other_member_list =
            typename object_impl<member<OtherKeys, OtherTs>...>::member_list;
        // Use the intersection of keys here.
        using all_keys = meta::union_t<member_list, other_member_list, member_less>;

        using missing_keys = meta::difference_t<all_keys, other_member_list, member_less>;

        using common_keys =
            meta::intersection_t<member_list, other_member_list, member_less>;
        using common_obj_t = meta::instantiate_t<object, common_keys>;
        common_obj_t::for_each_key([&]<auto key>() {
            get<key>() = rhs.template get<key>();
        });

        using extra_keys = meta::difference_t<all_keys, member_list, member_less>;
        using extra_obj_t = meta::instantiate_t<object, extra_keys>;
        extra_obj_t::for_each_key([&]<auto key>() {
            m_dynamic.emplace(std::string_view(key), rhs.template get<key>());
            std::cerr << "Key " << std::string_view(key)
                      << " does not belong to new object\n";
        });
    }

    template<fixed_string key>
    constexpr auto& get() {
        constexpr auto index = meta::index_of<key, keys...>();
        static_assert(index >= 0, "The given key is not a member name!");
        return std::get<index>(m_members);
    }

    template<fixed_string key>
    constexpr auto& get() const {
        constexpr auto index = meta::index_of<key, keys...>();
        static_assert(index >= 0, "The given key is not a member name!");
        return std::get<index>(m_members);
    }

    template<class T>
    static void for_each_key(T&& t) {
        (t.template operator()<keys>(), ...);
    }

    template<class T>
    void for_each_member(T&& t) const {
        (t.template operator()<keys>(get<keys>()), ...);
    }

    template<class T>
    void for_each_member(T&& t) {
        (t.template operator()<keys>(get<keys>()), ...);
    }

private:
    nlohmann::json m_dynamic;
    std::tuple<Ts...> m_members;
};

namespace detail {
template<class T, class T2>
struct object_and_impl;

template<fixed_string... left_keys,
         class... left_Ts,
         fixed_string... right_keys,
         class... right_Ts>
struct object_and_impl<object_impl<member<left_keys, left_Ts>...>,
                       object_impl<member<right_keys, right_Ts>...>> {
    using type = object<member<left_keys, left_Ts>..., member<right_keys, right_Ts>...>;
};
} // namespace detail

template<class T>
concept Object = meta::is_instance<T, object_impl>::value;

static_assert(Object<object<>>);
static_assert(Object<object<member<"", int>>>);

template<Object T, Object T2>
using object_and = typename detail::object_and_impl<T, T2>::type;


template<fixed_string Key, Object Obj>
decltype(auto) get(Obj& obj) {
    return obj.template get<Key>();
}

template<fixed_string Key, Object Obj>
decltype(auto) get(const Obj& obj) {
    return obj.template get<Key>();
}
} // namespace v60
