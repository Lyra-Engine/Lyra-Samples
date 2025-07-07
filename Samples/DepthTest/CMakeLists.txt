# resources
cmrc_add_resource_library(
    depth-test-resources
    shader.slang
    NAMESPACE resources)

# packages
find_package(Lyra-Engine REQUIRED)

# executable
add_lyra_executable(depth-test)
target_sources(depth-test PRIVATE main.cpp)
target_link_libraries(depth-test PRIVATE depth-test-resources)
target_link_libraries(depth-test PRIVATE lyra::engine)

# IDE support
set_target_properties(depth-test PROPERTIES FOLDER "Samples")
set_target_properties(depth-test-resources PROPERTIES FOLDER "Resources")
