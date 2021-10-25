cmake_minimum_required(VERSION 3.6)
project(hyperion-webos VERSION 1.0 LANGUAGES C)

add_subdirectory(tv-native-apis)
add_subdirectory(flatccrt)
add_subdirectory(tv-native-apis/samples)

find_package(PkgConfig REQUIRED)

pkg_check_modules(OpenGL REQUIRED IMPORTED_TARGET egl glesv2)

add_executable(hyperion-webos_libvtcapture
    src/capture_all.c
    src/hyperion_client.c
 )

 add_executable(hyperion-webos_libvt
 src/capture.c
 src/renderer.c
 src/hyperion_client.c
)

target_include_directories(hyperion-webos_libvt PRIVATE fbs incbin)
target_compile_definitions(hyperion-webos_libvt PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(hyperion-webos_libvt vt gm flatccrt pthread PkgConfig::OpenGL)

target_include_directories(hyperion-webos_libvtcapture PRIVATE fbs incbin)
target_compile_definitions(hyperion-webos_libvtcapture PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(hyperion-webos_libvtcapture vtcapture halgal flatccrt pthread)