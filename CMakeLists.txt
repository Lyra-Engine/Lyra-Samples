cmake_minimum_required(VERSION 3.28)
project(Lyra-Samples VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# use only in-house cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Configs")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Vendors")

# resource compiler
include(CMakeRC)

# samples
add_subdirectory(Samples/Window)
add_subdirectory(Samples/Triangle)
