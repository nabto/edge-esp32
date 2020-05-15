set(root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../nabto-embedded-sdk)



set(platform_src
  ${root_dir}/src/platform/np_error_code.c
  ${root_dir}/src/platform/np_event_queue.c
  ${root_dir}/src/platform/np_ip_address.c
  ${root_dir}/src/platform/np_logging.c
  ${root_dir}/src/platform/np_platform.c
  ${root_dir}/src/platform/np_udp.c
  ${root_dir}/src/platform/np_util.c
  ${root_dir}/src/platform/np_vector.c
  ${root_dir}/src/platform/np_list.c
  )

set(core_src
  ${root_dir}/src/core/nc_attacher.c
  ${root_dir}/src/core/nc_attacher_attach_end.c
  ${root_dir}/src/core/nc_attacher_attach_start.c
  ${root_dir}/src/core/nc_attacher_sct.c
  ${root_dir}/src/core/nc_client_connection.c
  ${root_dir}/src/core/nc_client_connection_dispatch.c
  ${root_dir}/src/core/nc_coap.c
  ${root_dir}/src/core/nc_coap_client.c
  ${root_dir}/src/core/nc_coap_packet_printer.c
  ${root_dir}/src/core/nc_coap_rest_error.c
  ${root_dir}/src/core/nc_coap_server.c
  ${root_dir}/src/core/nc_device.c
  ${root_dir}/src/core/nc_keep_alive.c
  ${root_dir}/src/core/nc_packet.c
  ${root_dir}/src/core/nc_rendezvous.c
  ${root_dir}/src/core/nc_rendezvous_coap.c
  ${root_dir}/src/core/nc_stream.c
  ${root_dir}/src/core/nc_stream_manager.c
  ${root_dir}/src/core/nc_stun.c
  ${root_dir}/src/core/nc_stun_coap.c
  ${root_dir}/src/core/nc_udp_dispatch.c
  ${root_dir}/src/core/nc_version.c
  )

set(dtls_src
  ${root_dir}/src/modules/dtls/nm_dtls_timer.c
  ${root_dir}/src/modules/dtls/nm_dtls_cli.c
  ${root_dir}/src/modules/dtls/nm_dtls_srv.c
  ${root_dir}/src/modules/dtls/nm_dtls_util.c
  ${root_dir}/src/modules/dtls/nm_random.c
  )

set(streaming_src
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_congestion_control.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_flow_control.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_log.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_log_helper.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_memory.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_packet.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_util.c
  ${root_dir}/nabto-common-cpp/src/streaming/nabto_stream_window.c
  )

set(coap_src
  ${root_dir}/nabto-common-cpp/src/coap/nabto_coap.c
  ${root_dir}/nabto-common-cpp/src/coap/nabto_coap_client_impl.c
  ${root_dir}/nabto-common-cpp/src/coap/nabto_coap_server_impl.c
  ${root_dir}/nabto-common-cpp/src/coap/nabto_coap_server_impl_incoming.c
  )

set(cbor_src
  ${root_dir}/3rdparty/tinycbor/tinycbor/src/cborencoder.c
  ${root_dir}/3rdparty/tinycbor/tinycbor/src/cborparser.c
  ${root_dir}/3rdparty/tinycbor/tinycbor/src/cborparser_dup_string.c
  ${root_dir}/3rdparty/tinycbor/tinycbor/src/cbortojson.c
  ${root_dir}/3rdparty/tinycbor/tinycbor/src/cborvalidation.c
  ${root_dir}/3rdparty/tinycbor/extra/cbor_encode_encoded_cbor.c
  ${root_dir}/3rdparty/tinycbor/extra/cbor_extra.c
  )

set(stun_src
  ${root_dir}/nabto-common-cpp/src/stun/nabto_stun_client.c
  ${root_dir}/nabto-common-cpp/src/stun/nabto_stun_log.c
  ${root_dir}/nabto-common-cpp/src/stun/nabto_stun_message.c
  )

set(mdns_src
  ${root_dir}/nabto-common-cpp/src/mdns/mdns_server.c
  )

set(api_src
  ${root_dir}/src/api/nabto_device_authorization.c
  ${root_dir}/src/api/nabto_api_future_queue.c
  ${root_dir}/src/api/nabto_device.c
  ${root_dir}/src/api/nabto_device_coap.c
  ${root_dir}/src/api/nabto_device_error.c
  ${root_dir}/src/api/nabto_device_event_handler.c
  ${root_dir}/src/api/nabto_device_experimental.c
  ${root_dir}/src/api/nabto_device_future.c
  ${root_dir}/src/api/nabto_device_stream.c
  ${root_dir}/src/api/nabto_device_util.c
  ${root_dir}/src/api/nabto_platform.c
  ${root_dir}/src/api/unix/nabto_device_threads_unix.c
  )

set(timestamp_src
  ${root_dir}/src/modules/timestamp/unix/nm_unix_timestamp.c
  )

set(communication_buffer_src
  ${root_dir}/src/modules/communication_buffer/nm_unix_communication_buffer.c
  )

set(api_logging_src
  ${root_dir}/src/modules/logging/api/nm_api_logging_stdout_unix.c
  ${root_dir}/src/modules/logging/api/nm_api_logging.c
  )

set(tcp_tunnels_src
  ${root_dir}/src/modules/tcp_tunnel/nm_tcp_tunnel_coap.c
  ${root_dir}/src/modules/tcp_tunnel/nm_tcp_tunnel.c
  ${root_dir}/src/modules/tcp_tunnel/nm_tcp_tunnel_connection.c
  )


set(adapter_src
  src/nm_select_unix.c
  src/nm_select_unix_udp.c
  src/nm_select_unix_tcp.c
  src/platform_modules_esp32.c
  src/esp32_dns.c
  src/esp32_logging.c
  )

idf_component_register(
  SRCS
    "${platform_src}"
    "${core_src}"
    "${streaming_src}"
    "${coap_src}"
    "${cbor_src}"
    "${cbor_extra_src}"
    "${dtls_src}"
    "${stun_src}"
    "${mdns_src}"
    "${api_src}"
    "${adapter_src}"
    "${timestamp_src}"
    "${communication_buffer_src}"
    "${api_logging_src}"
    "${tcp_tunnels_src}"
  INCLUDE_DIRS
    ${root_dir}/include
  PRIV_INCLUDE_DIRS
    ${root_dir}/src
    ${root_dir}/nabto-common-cpp/src
    ${root_dir}/3rdparty/tinycbor/extra
    ${root_dir}/3rdparty/tinycbor/tinycbor/src
    .
  REQUIRES mbedtls)

add_custom_target(GENERATE_NABTO_VERSION ALL
  WORKING_DIRECTORY ${root_dir}/src/core
  BYPRODUCTS ${root_dir}/src/core/nc_version.c
  COMMAND ${CMAKE_COMMAND} -P
  ${root_dir}/src/core/version.cmake
  )

add_dependencies(${COMPONENT_LIB} GENERATE_NABTO_VERSION)
