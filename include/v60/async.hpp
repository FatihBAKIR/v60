#pragma once

#include <boost/asio/awaitable.hpp>

namespace v60 {
template<class T>
using task = boost::asio::awaitable<T>;
}