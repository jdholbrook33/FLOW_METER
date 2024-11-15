
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES wifi_manager flow_sensor rtc_manager storage_manager web_server
)

