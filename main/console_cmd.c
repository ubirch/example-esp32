/*!
 * #file    console_cmd.c
 * #brief   Custom Console commands.
 *
 * #author Waldemar Gruenwald
 * #date   2018-11-14
 *
 * #copyright &copy; 2018 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */


#include <esp_log.h>
#include <esp_console.h>
#include <argtable3/argtable3.h>
//#include <esp_err.h>
#include "util.h"

#include "key_handling.h"
#include "wifi.h"


#include "console_cmd.h"
#include "storage.h"
#include "sntp_time.h"

/*
 * 'exit' command exits the console and runs the rest of the program
 */
static int exit_console(int argc, char **argv) {
    ESP_LOGI(__func__, "Exiting Console");
    return EXIT_CONSOLE;
}

void register_exit() {
    const esp_console_cmd_t cmd = {
            .command = "exit",
            .help = "Exit the console and resume with program",
            .hint = NULL,
            .func = &exit_console,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/*
 * 'run' command runs the rest of the program
 */
static int run_status(int argc, char **argv) {
    ESP_LOGI(__func__, "Current Status\r\n");
    struct Wifi_login wifi;
    char buffer[65] = {};
    // show the Hardware device ID
    get_hw_ID();
    // show the Public Key, if available
    if (!get_public_key(buffer)) {
        ESP_LOGI("Public Key not available", "");
    } else {
        ESP_LOGI("Public Key", "%s", buffer);
    }
    // show the wifi login information, if available
    if (!load_wifi_login(&wifi)) {
        ESP_LOGI("Wifi SSID", "%s", wifi.ssid);
        ESP_LOGD("Wifi PWD", "%s", wifi.pwd);
    } else {
        ESP_LOGI("Wifi not configured yet", "type join to do so");
    }
    time_status();

    return ESP_OK;
}

void register_status() {
    const esp_console_cmd_t cmd = {
            .command = "status",
            .help = "Get the current status of the system",
            .hint = NULL,
            .func = &run_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/*
 * 'join' command to connect with wifi network
 */

/*
 * Arguments used by 'join' function
 */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

static int connect(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }

    struct Wifi_login wifi;
    strncpy(wifi.ssid, join_args.ssid->sval[0], strlen(join_args.ssid->sval[0]));
    wifi.ssid_length = strlen(join_args.ssid->sval[0]);
    strncpy(wifi.pwd, join_args.password->sval[0], strlen(join_args.password->sval[0]));
    wifi.pwd_length = strlen(join_args.password->sval[0]);

    ESP_LOGI(__func__, "Connecting to '%s'",
             join_args.ssid->sval[0]);


    bool connected = wifi_join(wifi,
                               join_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    // Store the wifi login data

    store_wifi_login(wifi);
    return 0;
}

void register_wifi() {
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.timeout->ival[0] = 5000; // set default value
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
            .command = "join",
            .help = "Join WiFi AP as a station",
            .hint = NULL,
            .func = &connect,
            .argtable = &join_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
}

