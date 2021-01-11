cmake_minimum_required(VERSION 3.13)

project(v60 CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ROOT "C:\\local\\boost_1_75_0")
find_package(Boost REQUIRED COMPONENTS system date_time)

add_library(v60
    src/routing.cpp
    src/meta.cpp
    src/object.cpp
    src/async.cpp
    src/fixed_string.cpp
    include/ctre.hpp
    include/v60/fixed_string.hpp
    include/v60/meta.hpp
    include/v60/object.hpp
    include/v60/request.hpp
    include/v60/ctre.hpp
    include/v60/routing.hpp
    include/v60/async.hpp
    include/v60/group.hpp
    include/v60/binding.hpp
    include/v60/end_point.hpp
    include/v60/middleware.hpp
    include/v60/response.hpp
    include/v60/http.hpp
    include/v60/server.hpp
    src/server.cpp
    include/v60/v60.hpp
)
target_compile_features(v60 PUBLIC cxx_std_20)
target_include_directories(v60 PUBLIC include)
target_link_libraries(v60 PUBLIC Boost::boost Boost::system Boost::date_time)
target_link_directories(v60 PUBLIC ${Boost_LIBRARY_DIRS})

find_package(Threads REQUIRED)
target_link_libraries(v60 PUBLIC Threads::Threads)

if (WIN32)
    target_compile_definitions(v60 PUBLIC _WIN32_WINNT=0x0601)
else()
    target_compile_options(v60 PUBLIC -stdlib=libc++)
    target_link_options(v60 PUBLIC -stdlib=libc++)
    target_link_options(v60 PUBLIC -fuse-ld=lld)
endif()

add_subdirectory(examples)
