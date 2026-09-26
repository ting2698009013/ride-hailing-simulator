cmake_minimum_required(VERSION 3.16)
project(ride_hailing_core_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This target exercises the simulator and hand-written data structures without
# linking the EasyX user interface, so it can run on Windows and Linux CI.
add_executable(ride_hailing_tests
    tests/headless_main.cpp
    src/tests/SelfTest.cpp
    src/core/CityGrid.cpp
    src/core/DispatchEngine.cpp
    src/core/Simulator.cpp
)
target_include_directories(ride_hailing_tests PRIVATE src)

if(MSVC)
    target_compile_options(ride_hailing_tests PRIVATE /W4 /utf-8)
else()
    target_compile_options(ride_hailing_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
