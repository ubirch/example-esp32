//
// Created by wowa on 12.12.18.
//

#include <esp_log.h>
#include <ssd1306.h>
#include "oled.h"

void oled_show() {
    if (ssd1306_init(0, 22, 23)) {
        ESP_LOGI("OLED", "oled inited");
        ssd1306_draw_rectangle(0, 0, 0, 128, 32, 1);
        ssd1306_select_font(0, 0);
        ssd1306_draw_string(0, 1, 1, "hallo stephan", 1, 0);
        ssd1306_select_font(0, 1);
        ssd1306_draw_string(0, 1, 18, "so gehts", 1, 0);
        ssd1306_refresh(0, true);
    } else {
        ESP_LOGE("OLED", "oled init failed");
    }
}

