set(COMPONENT_SRCS
        networking_wifi.c
        sntp_time.c
        )
set(COMPONENT_ADD_INCLUDEDIRS ".")
#set(COMPONENT_PRIV_REQUIRES esp32 lwip)
register_component()