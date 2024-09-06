set(ne_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../nabto-embedded-sdk)
set(src_dir ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_primary_files.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../nabto-embedded-sdk/nabto_modules_files.cmake)

# Implementation for nabto_device_threads.h
set(ne_unix_threads ${ne_dir}/src/modules/threads/unix/nabto_device_threads_unix.c)

# MBEDTLS wrapper functions for crypto
set(ne_mbedtls_util ${ne_dir}/src/modules/mbedtls/nm_mbedtls_util.c)



# Nabto select unix impl. unix adopted to esp32
set(ne_esp32_dir ./src)
set(ne_select_esp32_src
  ${ne_esp32_dir}/nm_select_unix.c
  ${ne_esp32_dir}/nm_select_unix_udp.c
  ${ne_esp32_dir}/nm_select_unix_tcp.c
  ${ne_esp32_dir}/platform_modules_esp32.c
  ${ne_esp32_dir}/esp32_logging.c
  ${ne_esp32_dir}/esp32_event_queue.c
)

set(ne_esp32_dns_src
  ${ne_esp32_dir}/esp32_dns.c
)

set(ne_esp32_mdns_src
  ${ne_esp32_dir}/esp32_mdns.c
)

idf_component_register(
  SRCS
    ${ne_utils_src}
    ${ne_streaming_src}
    ${ne_coap_src}
    ${ne_platform_src}
    ${ne_core_src}
    ${ne_api_src}
    ${ne_api_future_queue_src}
    ${ne_nn_src}
    ${ne_mbedtls_module_src}
    ${ne_tcp_tunnels_src}
    ${ne_communication_buffer_src}
    ${ne_unix_threads}
    ${ne_select_esp32_src}
    ${ne_select_unix_example}
    ${ne_event_queue_src}
    ${ne_thread_event_queue_src}
    ${ne_nn_common_components_src}
    ${ne_timestamp_src}
    ${ne_localip_src}
    ${ne_tcp_tunnel_src}
    ${ne_esp32_logging}
    ${ne_esp32_dns_src}
    ${ne_esp32_mdns_src}
  INCLUDE_DIRS
    ${ne_coap_include_dirs}
    ${ne_streaming_include_dirs}
    ${ne_stun_include_dirs}
    ${ne_nn_include_dirs}
    ${ne_include_dirs}
  PRIV_INCLUDE_DIRS
    include
  REQUIRES mbedtls pthread mdns)

target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DHAVE_ARPA_INET_H -DNABTO_DEVICE_DTLS_CLIENT_ONLY")

add_custom_target(GENERATE_NABTO_VERSION ALL
  WORKING_DIRECTORY ${ne_dir}/src/core
  BYPRODUCTS ${ne_dir}/src/core/nc_version.c
  COMMAND ${CMAKE_COMMAND} -P
  ${ne_dir}/src/core/version.cmake
  )

add_dependencies(${COMPONENT_LIB} GENERATE_NABTO_VERSION)
