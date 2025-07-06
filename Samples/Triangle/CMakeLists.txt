# resources
cmrc_add_resource_library(
    triangle-resources
    shader.slang
    NAMESPACE resources)

# packages
find_package(Lyra-Engine REQUIRED)

# executable
add_lyra_executable(triangle)
target_sources(triangle PRIVATE main.cpp)
target_link_libraries(triangle PRIVATE triangle-resources)
target_link_libraries(triangle PRIVATE lyra::engine)

# IDE support
set_target_properties(triangle PROPERTIES FOLDER "Samples")
set_target_properties(triangle-resources PROPERTIES FOLDER "Resources")
