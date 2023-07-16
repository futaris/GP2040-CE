add_library(picow_access_point
picow_access_point.c
dhserver.c
dnserver.c
)

target_include_directories(picow_access_point PUBLIC
${CMAKE_CURRENT_LIST_DIR}
${CMAKE_SOURCE_DIR}/headers
${PICO_SDK_PATH}/lib/lwip/src/include/lwip/apps
${PICO_SDK_PATH}/lib/lwip/src/include
)

target_link_libraries(picow_access_point
pico_cyw43_arch_lwip_threadsafe_background
pico_stdlib
lwip-port
httpd
)
