

include(../../nabto-embedded-sdk/nabto_primary_files.cmake)
include(../../nabto-embedded-sdk/nabto_modules_files.cmake)

message("nn dir:" ${ne_nn_common_components_include})
message("nn dir:" ${ne_nn_common_components_include})


idf_component_register(
  SRCS
    tcp_tunnel_main.c
    simple_webserver.c
    device_event_handler.c
    device_config.c
    iam_config.c
    json_config.c
    logging.c
    private_key.c
    tcp_tunnel_state.c
    random_string.c
    ${ne_iam_src}
    ${ne_policies_src}
  INCLUDE_DIRS
    ../../nabto-embedded-sdk/src
    ../../nabto-embedded-sdk/3rdparty/cjson
    ../../common_components/nabto_device/src    
    ${ne_include_dirs}
    ${ne_nn_common_components_include}    
    ${ne_priv_include_dirs}
)


#    ../../nabto-embedded-sdk/nabto-common/components/nn/include
#    ../../nabto-embedded-sdk/include
#    ../../nabto-embedded-sdk/src
