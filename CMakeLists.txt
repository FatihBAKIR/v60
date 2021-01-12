cmake_minimum_required(VERSION 3.10)

project(v60 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(V60_INSTALL "Enable installing v60" ON)
option(V60_EXAMPLES "Enable building examples v60" ON)
option(V60_NO_VENDOR "Disable vendored libraries" OFF)

set(Boost_USE_STATIC_LIBS ON)
set(BOOST_ROOT "C:\\local\\boost_1_75_0")
find_package(Boost REQUIRED COMPONENTS system date_time)

add_library(v60
    src/routing.cpp
    src/meta.cpp
    src/object.cpp
    src/async.cpp
    src/fixed_string.cpp
    include/v60/internal/ctre.hpp
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
    src/end_point.cpp
    src/simdjson.cpp
)
target_compile_features(v60 PUBLIC cxx_std_20)
target_include_directories(v60 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(v60 PUBLIC Boost::boost Boost::system Boost::date_time)
target_link_directories(v60 PUBLIC ${Boost_LIBRARY_DIRS})

if(${CMAKE_VERSION} VERSION_GREATER "3.16.0")
    target_precompile_headers(v60 PUBLIC include/v60/v60.hpp)
endif()

if (V60_NO_VENDOR)
    target_sources(v60 PRIVATE src/simdjson.cpp)
endif()

find_package(Threads REQUIRED)
target_link_libraries(v60 PUBLIC Threads::Threads)

if (WIN32)
    target_compile_definitions(v60 PUBLIC _WIN32_WINNT=0x0601)
else()
    target_compile_options(v60 PUBLIC -stdlib=libc++)
    target_link_options(v60 PUBLIC -stdlib=libc++ -fuse-ld=lld -lc++abi)
endif()

if (V60_EXAMPLES)
    add_subdirectory(examples)
endif()

if (V60_INSTALL)
    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
    install(
            TARGETS v60
            EXPORT v60
            DESTINATION lib
    )

    install(DIRECTORY include/v60 DESTINATION include)
    install(EXPORT v60 DESTINATION lib/cmake/v60)
    install(FILES v60-config.cmake DESTINATION lib/cmake/v60)

    set(CPACK_GENERATOR "DEB;RPM;TGZ;STGZ")
    set(CPACK_PACKAGE_VENDOR "Fatih")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "v60")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fatih Bakir")
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/Readme.adoc")
    include(CPack)
endif()

