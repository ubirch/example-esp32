//
// Created by wowa on 13.11.18.
//

#ifndef EXAMPLE_ESP32_UBIRCH_CONSOLE_H
#define EXAMPLE_ESP32_UBIRCH_CONSOLE_H

#if CONFIG_STORE_HISTORY
void initialize_filesystem();
#endif // CONFIG_STORE_HISTORY

void initialize_nvs();

void initialize_console();

void run_console(void);


#endif //EXAMPLE_ESP32_UBIRCH_CONSOLE_H
