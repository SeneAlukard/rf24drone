cmake_minimum_required(VERSION 3.12)
project(drone LANGUAGES CXX)

# Set C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for tooling (e.g., clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add header files (include/)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect all .cpp source files in src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

# Define the executable target
add_executable(drone ${SRC_FILES})

# Link with RF24 (assumed to be installed system-wide)
target_link_libraries(drone PRIVATE rf24)

# Optionally create symlink for compile_commands.json at project root
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    RESULT_VARIABLE link_result
  )
endif()

# Add a custom 'run' target to build and execute
add_custom_target(run
  DEPENDS drone
  COMMAND drone
  COMMENT "🚀 Running drone"
)

