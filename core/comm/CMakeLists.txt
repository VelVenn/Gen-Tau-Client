file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

gt_register_mod(
  NAME comm
  TYPE STATIC
  SRC ${SRC_FILES}
  DEPS
    utils
    PahoMqttCpp::paho-mqttpp3-static

    PRIVATE
      conf
)