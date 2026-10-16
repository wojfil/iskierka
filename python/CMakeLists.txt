cmake_minimum_required(VERSION 3.18)

project(iskierka_python LANGUAGES CXX)
find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(iskierka bindings.cpp)
target_compile_features(iskierka PRIVATE cxx_std_17)
target_include_directories(iskierka PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../cpp"
)
install(TARGETS iskierka DESTINATION .)
