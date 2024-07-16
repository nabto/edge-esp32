

idf_component_register(SRCS "tcp_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES tcp_server_stream audio_pipeline audio_hal audio_stream esp_peripherals protocol_examples_common)

