# CMakeLists.txt (top-level)
cmake_minimum_required(VERSION 3.15)
project(AnalysisSynthesizer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(ROOT REQUIRED COMPONENTS RIO Tree Hist)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)         # optional logging

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB SRC src/*.cpp src/processors/*.cpp)

add_executable(synthesizer src/main.cpp ${SRC})
target_link_libraries(synthesizer
  PRIVATE ROOT::RIO ROOT::Tree ROOT::Hist
          yaml-cpp::yaml-cpp
          spdlog::spdlog)

# install, testing, etc.
