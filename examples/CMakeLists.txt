add_executable(backend backend.cpp)
target_link_libraries(backend PUBLIC v60)

add_executable(barebones barebones.cpp)
target_link_libraries(barebones PUBLIC v60)

add_executable(parameter parameter.cpp)
target_link_libraries(parameter PUBLIC v60)
