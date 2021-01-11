add_library(simdjson simdjson.cpp)
target_include_directories(simdjson PUBLIC .)
target_compile_features(simdjson PUBLIC cxx_std_20)