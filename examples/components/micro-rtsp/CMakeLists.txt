idf_component_register(
  SRCS CRtspSession.cpp CStreamer.cpp OV2640.cpp OV2640Streamer.cpp
  INCLUDE_DIRS .
  REQUIRES esp32-camera
)
