#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H


//#define WEB_PORT "80"
//#define UHTTP_HOST "unsafe.api.ubirch.dev.ubirch.com"
//#define UHTTP_URL "http://unsafe.api.ubirch.dev.ubirch.com/api/avatarService/v1/device/update/mpack"

//static const char *REQUEST = "POST " UHTTP_URL " HTTP/1.0\r\n"
//        "Host: "UHTTP_HOST"\r\n"
//        "User-Agent: esp-idf/1.0 esp32\r\n"
//        "\r\n";

///* Constants that aren't configurable in menuconfig */
//#define UHTTP_HOST "192.168.178.60"
//#define WEB_PORT "3333"
//#define UHTTP_URL "https://" UHTTP_HOST
//
//static const char *REQUEST = "GET " UHTTP_URL " HTTP/1.0\r\n"
//        "Host: "UHTTP_HOST"\r\n"
//        "User-Agent: esp-idf/1.0 esp32\r\n"
//        "\r\n";


void https_get_task(void *pvParameters);

void tcp_client_send_msg(char *msg, uint16_t msg_length);

#endif /* NETWORK_UTILS_H */