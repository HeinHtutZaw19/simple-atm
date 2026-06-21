cmake_minimum_required(VERSION 3.14)
project(simple_atm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
endif()

# --- PRODUCTION CODE ---
add_library(atm
   src/ATMController.cpp
)
target_include_directories(
    atm
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
   target_compile_options(atm PRIVATE /W4)
else()
   target_compile_options(atm PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- TESTS ---
enable_testing()

add_executable(atm_tests
   tests/test_controller.cpp
)
target_link_libraries(atm_tests PRIVATE atm)
target_include_directories(
    atm_tests
    PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/third_party
   ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
add_test(NAME atm_tests COMMAND atm_tests)