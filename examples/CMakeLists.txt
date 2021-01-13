add_executable(backend backend.cpp)
target_link_libraries(backend PUBLIC v60)

add_executable(barebones barebones.cpp)
target_link_libraries(barebones PUBLIC v60)

add_executable(parameter parameter.cpp)
target_link_libraries(parameter PUBLIC v60)

add_executable(code404 404.cpp)
target_link_libraries(code404 PUBLIC v60)

add_executable(code500 500.cpp)
target_link_libraries(code500 PUBLIC v60)

add_executable(cookies cookies.cpp)
target_link_libraries(cookies PUBLIC v60)

add_executable(static static.cpp)
target_link_libraries(static PUBLIC v60)
