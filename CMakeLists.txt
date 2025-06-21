cmake_minimum_required(VERSION 3.16)
project(drone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# compile_commands.json çıktısı
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- YENİ EKLENEN BÖLÜM ---
# RF24 submodule'ünün CMakeLists.txt dosyasını işle
# Bu, RF24 kütüphanesini derleyecek ve bir CMake hedefi oluşturacaktır.
# external/RF24 klasörünün projenizin ana dizininde olduğunu varsayıyoruz.
add_subdirectory(external/RF24)
# --- YENİ EKLENEN BÖLÜM SONU ---

# src altındaki tüm cpp dosyalarını al
add_library(drone_core
    src/drone.cpp
    src/radio.cpp
    src/mpu6050.cpp
)

add_executable(drone src/main.cpp)

target_include_directories(drone_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/RF24
)

target_include_directories(drone
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/RF24
)

# --- SİLİNEN VEYA DEĞİŞTİRİLEN BÖLÜM ---
# # RF24 sistemde kuruluysa
# find_library(RF24_LIB NAMES rf24)
# if(NOT RF24_LIB)
#     message(FATAL_ERROR "RF24 library not found")
# endif()
#
# target_link_libraries(drone PRIVATE ${RF24_LIB})
# --- SİLİNEN VEYA DEĞİŞTİRİLEN BÖLÜM SONU ---

# --- YENİ target_link_libraries SATIRI ---
# drone hedefini, submodule tarafından oluşturulan rf24 hedefine bağla
# (RF24'ün CMake hedef adının "rf24" olduğunu varsayıyoruz)
target_link_libraries(drone_core PUBLIC rf24)
target_link_libraries(drone PRIVATE drone_core)
# --- YENİ target_link_libraries SATIRI SONU ---

# compile_commands.json symlink
add_custom_command(
    TARGET drone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# make run hedefi
add_custom_target(run
    COMMAND drone
 DEPENDS drone
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Place test executables under the project root in a `test` directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/test)

add_executable(duplex_test duplex_test.cpp)
target_link_libraries(duplex_test PRIVATE drone_core)
set_target_properties(duplex_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/test
)

add_executable(packets_test packets_test.cpp)
target_link_libraries(packets_test PRIVATE drone_core)
set_target_properties(packets_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/test
)

# Symlink the main binary to the project root for convenience
add_custom_command(
    TARGET drone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE:drone>
            ${CMAKE_SOURCE_DIR}/drone
)
