cmake_minimum_required(VERSION 3.6)
project(hyperion-webos_libvtcapture VERSION 1.0 LANGUAGES C)

add_subdirectory(tv-native-apis)
add_subdirectory(flatccrt)

add_executable(hyperion-webos_libvtcapture
    src/capture_libvtcapture_libhalgal.c
    src/hyperion_client.c
 )

target_include_directories(hyperion-webos_libvtcapture PRIVATE fbs)
target_compile_definitions(hyperion-webos_libvtcapture PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(hyperion-webos_libvtcapture vtcapture halgal flatccrt pthread)