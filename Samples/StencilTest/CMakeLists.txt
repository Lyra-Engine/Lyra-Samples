# resources
cmrc_add_resource_library(
    stencil-test-resources
    shader.slang
    NAMESPACE resources)

# packages
find_package(Lyra-Engine REQUIRED)

# executable
add_lyra_executable(stencil-test)
target_sources(stencil-test PRIVATE main.cpp)
target_link_libraries(stencil-test PRIVATE stencil-test-resources)
target_link_libraries(stencil-test PRIVATE lyra::engine)

# IDE support
set_target_properties(stencil-test PROPERTIES FOLDER "Samples")
set_target_properties(stencil-test-resources PROPERTIES FOLDER "Resources")
