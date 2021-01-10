#pragma once

#include <boost/asio/awaitable.hpp>


#if !defined(BOOST_ASIO_HAS_CO_AWAIT)
#error "Need co_await!"
#endif


namespace v60 {
template<class T>
using task = boost::asio::awaitable<T>;
}