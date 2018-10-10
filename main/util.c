
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>

#include "settings.h"
#include "util.h"

unsigned char UUID[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x23,
                          0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9A, 0xAB
};


/*
 * print out hex data on console output
 */
void print_message(const char *data, size_t size){
    printf("MESSAGE: ");
    int i = 0;
    for (i = 0; i < size; ++i) {
        printf("%02x",data[i]);
    }
    printf("\r\n");
}

void getSetUUID(void){
    esp_efuse_mac_get_default(UUID);
    esp_base_mac_addr_set(UUID);
    ESP_LOGI("UUID", "ID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
             UUID[0],UUID[1],UUID[2], UUID[3], UUID[4],UUID[5],UUID[6],UUID[7],
             UUID[8], UUID[9], UUID[10],UUID[11],UUID[12], UUID[13], UUID[14],UUID[15]);
}
