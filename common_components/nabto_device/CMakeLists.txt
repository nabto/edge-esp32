set(ne_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../nabto-embedded-sdk)
set(src_dir ${CMAKE_CURRENT_SOURCE_DIR})

include(../../nabto-embedded-sdk/nabto_primary_files.cmake)
include(../../nabto-embedded-sdk/nabto_modules_files.cmake)

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
  ${ne_esp32_dir}/select_unix_event_queue.c
  ${ne_esp32_dir}/esp32_logging.c
)

set(ne_esp32_dns_src
  ${ne_esp32_dir}/esp32_dns.c
)

set(ne_esp32_mdns_src
  #${ne_esp32_dir}/esp32_mdns.c
)

idf_component_register(
  SRCS
    ${ne_required}
    ${ne_unix_threads}
    ${ne_mbedtls_util}
    ${ne_select_esp32_src}
    ${ne_select_unix_example}
    ${ne_dtls_cli_src}
    ${ne_dtls_srv_src}
    ${ne_mbedtls_random_src}
    ${ne_event_queue_src}
    ${ne_communication_buffer_src}
    ${ne_nn_common_components_src}
    ${ne_timestamp_src}
    ${ne_localip_src}
    ${ne_tcp_tunnel_src}
    ${ne_esp32_logging}
    ${ne_esp32_dns_src}
    ${ne_esp32_mdns_src}
  INCLUDE_DIRS
    ${ne_include_dirs}
  PRIV_INCLUDE_DIRS
    ${ne_priv_include_dirs}
    ${ne_nn_common_components_include}
    .
  REQUIRES mbedtls)

    #${ne_dns_src}
    #${ne_mdns_src}

target_compile_definitions(${COMPONENT_TARGET} PUBLIC "-DHAVE_ARPA_INET_H")

add_custom_target(GENERATE_NABTO_VERSION ALL
  WORKING_DIRECTORY ${ne_dir}/src/core
  BYPRODUCTS ${ne_dir}/src/core/nc_version.c
  COMMAND ${CMAKE_COMMAND} -P
  ${ne_dir}/src/core/version.cmake
  )

add_dependencies(${COMPONENT_LIB} GENERATE_NABTO_VERSION)
