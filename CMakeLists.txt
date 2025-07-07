cmake_minimum_required(VERSION 3.28)
project(Lyra-Samples VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# use only in-house cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Configs")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Vendors")

# resource compiler
include(CMakeRC)

# backend selection
set(LYRA_BACKEND "Vulkan" CACHE STRING "Graphics backend to use")
set_property(CACHE LYRA_BACKEND PROPERTY STRINGS "D3D12" "Vulkan" "Metal")
set(VALID_BACKENDS "D3D12" "Vulkan" "Metal")
if(NOT LYRA_BACKEND IN_LIST VALID_BACKENDS)
  message(FATAL_ERROR
      "Invalid LYRA_BACKEND: ${LYRA_BACKEND}\n"
      "Valid options are:\n"
      "  - D3D12\n"
      "  - Vulkan\n"
      "  - Metal")
endif()

if(LYRA_BACKEND STREQUAL "D3D12")
  message(STATUS "Using D3D12 backend!")
  add_compile_definitions(LYRA_RHI_BACKEND=RHIBackend::D3D12)
  add_compile_definitions(LYRA_RHI_COMPILER=CompileTarget::DXIL)
endif()

if(LYRA_BACKEND STREQUAL "Vulkan")
  message(STATUS "Using Vulkan backend!")
  add_compile_definitions(LYRA_RHI_BACKEND=RHIBackend::VULKAN)
  add_compile_definitions(LYRA_RHI_COMPILER=CompileTarget::SPIRV)
endif()

# NOTE: Metal is currently not supported, this is just a placeholder.
if(LYRA_BACKEND STREQUAL "Metal")
  message(STATUS "Using Metal backend!")
  message(FATAL_ERROR "Metal backend is currently not supported!")
  add_compile_definitions(LYRA_RHI_BACKEND=RHIBackend::METAL)
  add_compile_definitions(LYRA_RHI_COMPILER=CompileTarget::SPIRV)
endif()

# samples
add_subdirectory(Samples/Window)
add_subdirectory(Samples/Triangle)
add_subdirectory(Samples/DepthTest)
