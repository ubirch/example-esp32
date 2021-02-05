#include <stdio.h>
#include "unity.h"

void app_main(void)
{
    printf("Run all the registered tests.\n");
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    printf("Start interactive test menu.\n");
    unity_run_menu();
}
