//
// Created by wowa on 13.11.18.
//

#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_vfs_dev.h>
#include <driver/uart.h>
#include <esp_console.h>
#include <esp_log.h>
#include <linenoise/linenoise.h>
#include <sys/fcntl.h>
//#include "esp32-hal.h"

#include "ubirch_console.h"
#include "console_cmd.h"


/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

void initialize_filesystem()
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (*s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY


void initialize_console() {
    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .use_ref_tick = true
    };
    ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM,
                                        256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void run_console(void) {

    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    register_system();
    register_wifi();
    register_status();
    register_exit();

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char *prompt = LOG_COLOR(LOG_COLOR_CYAN) "esp32> " LOG_RESET_COLOR;

    printf("\n");
    printf("  #############,           ,#############\n"
           "  #########                     #########\n"
           "  ######                           ######\n"
           "  ####                               ####\n"
           "  ###        ######*       .###       ###\n"
           "  ##       ###*   ####    #######      ##\n"
           "  #       &##       ##*  *#*  .##       #\n"
           "          ##.  *#*  *#*  *#*  .##        \n"
           "          ##.  *#*  *#*  *#*  .##        \n"
           "          ##.  *#*  *#*  *#*  .##        \n"
           "          ##.  *#*  *#*  *#*  .##        \n"
           "          *#&  .##       ##.  &#*        \n"
           "           ##    ####*####    ##         \n"
           "  &         ###    #####    ###         &\n"
           "  ##         ####.       .####         ##\n"
           "  ##&           ###########           &##\n"
           "  ####                               ####\n"
           "  ######                           ######\n"
           "  ########&                     &########\n"
           "  ############&,           ,&############\n");
    printf("\n"
           "            UBIRCH-ESP-CONSOLE\n"
           "Type 'help' to get the list of commands.\n");
//                   "Use UP/DOWN arrows to navigate through command history.\n"
//                   "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
    }
    /* Flag to stay in the console */
    bool console_status = true;

    while (console_status) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char *line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
        /* Save command history to filesystem */
        linenoiseHistorySave(HISTORY_PATH);
#endif
        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK) {
            if (ret == EXIT_CONSOLE) {
                /* exit from the console and resume program */
                console_status = false;
            } else if (ret != ESP_OK) {
                printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
            }
        } else {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}