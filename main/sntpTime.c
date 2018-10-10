
#include <esp_log.h>
#include "apps/sntp/sntp.h"
#include <sched.h>
#include <freertos/task.h>
#include <time.h>
#include "sntpTime.h"

/*
 * Initialize the SNTP server
 */
void initialize_sntp(void) {
    char *TAG = "sntp";
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

/*
 * get the time from NTP server
 */
void obtain_time(void) {
    char *TAG = "time";
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2017 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    ESP_LOGI(TAG, "TIME = %02d.%02d.%04d %02d:%02d:%02d\r\n", timeinfo.tm_mday, timeinfo.tm_mon, (1900 + timeinfo.tm_year),
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    char strftime_buf[64];
    setenv("TZ", "CST-2", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Berlin is: %s", strftime_buf);
}

/*
 *
 */

#define TIME_RES_SEC 1000000
uint64_t getTimeUs(){
    char *TAG = "time in us";
    time_t now = time(NULL);
    int64_t timer = esp_timer_get_time();
    uint64_t time_us = ((uint64_t)(now) * TIME_RES_SEC) + (((uint64_t)(timer) *TIME_RES_SEC / 1000000) % TIME_RES_SEC);
    ESP_LOGI(TAG, "= %llu", time_us);
    return time_us;
}