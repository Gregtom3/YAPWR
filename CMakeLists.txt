cmake_minimum_required(VERSION 3.15)
project(AnalysisSynthesizer VERSION 0.1 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ALL of ROOT (no COMPONENTS list)
find_package(ROOT REQUIRED)

# Find yaml-cpp for your YAML parsing
find_package(yaml-cpp REQUIRED)

# Include your headers and ROOT headers
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${ROOT_INCLUDE_DIRS}
  src/processors
  src/errors
)

# Gather all your source files
file(GLOB SRC
  src/*.cpp
  src/processors/*.cpp
  src/errors/*.cpp
)

# Define the executable
add_executable(synthesizer
  src/main.cpp
  ${SRC}
)

# Link against every ROOT library plus yaml-cpp
target_link_libraries(synthesizer
  PRIVATE
    ${ROOT_LIBRARIES}
    yaml-cpp::yaml-cpp
)

# (Optional) Enable testing or installation steps here…
