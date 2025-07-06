# resources
cmrc_add_resource_library(
    window-resources
    shader.slang
    NAMESPACE resources)

# packages
find_package(Lyra-Engine REQUIRED)

# executable
add_lyra_executable(window)
target_sources(window PRIVATE main.cpp)
target_link_libraries(window PRIVATE window-resources)
target_link_libraries(window PRIVATE lyra::engine)

# IDE support
set_target_properties(window PROPERTIES FOLDER "Samples")
set_target_properties(window-resources PROPERTIES FOLDER "Resources")
