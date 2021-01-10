#pragma once

#include <boost/beast/http.hpp>


namespace v60 {
namespace http {
using boost::beast::http::verb;
using str_request = boost::beast::http::request<boost::beast::http::string_body>;
using str_response = boost::beast::http::response<boost::beast::http::string_body>;
} // namespace http
} // namespace v60