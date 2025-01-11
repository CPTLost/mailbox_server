/*******************************************************************
 * File Name         : main.c
 * Description       : Creates a Task that runs a UDP server which serves data to a HTTPS server and also displays 
 *
 * Author            : Noah Plank
 *
 ******************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdlib.h>

#include "wifi.h"
#include "led_matrix.h"
#include "udp_server.h"
#include "https_server.h"

#define DOOMSDAY 0
#define BLINKING_DELAY_MS 500

#define STD_STACK_SIZE 4096
#define STD_PRIO 3

static const char *TAG = "MAIN";

static TaskHandle_t led_blink_TaskHandle = NULL;
static TaskHandle_t udp_server_TaskHandle = NULL;

static bool mail_available = false;

void led_blink_task(void *param)
{
    initLedMatrix();
    while (!DOOMSDAY)
    {
        if (false == wifi_connected)
        {
            led_matrix_set_color_hsv(RED, 25);
            vTaskDelay(BLINKING_DELAY_MS / portTICK_PERIOD_MS);
            led_matrix_clear();
            vTaskDelay(BLINKING_DELAY_MS / portTICK_PERIOD_MS);
        }
        if ((true == wifi_connected) && (false == mail_available))
        {
            led_matrix_set_color_hsv(BLUE, 25);
        }
        else if ((true == wifi_connected) && (true == mail_available))
        {
            led_matrix_set_color_hsv(GREEN, 25);
        }
        else
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink_task", STD_STACK_SIZE, NULL, STD_PRIO, &led_blink_TaskHandle);
    // wifi_init_sta waits until wifi is connected or failed to connect
    if (CONNECTION_FAILED == wifi_init_sta())
    {
        ESP_LOGE(TAG, "Something went very wrong... wifi_init_sta failed");
    }
    else
    {
        udp_server_task_param_t *udp_server_task_param = malloc(sizeof(udp_server_task_param_t));
        if (NULL == udp_server_task_param)
        {
            ESP_LOGE(TAG, "malloc for udp_server_task_param failed");
            return;
        }
        else
        {
            udp_server_task_param->mail_available = &mail_available;
            // When wifi is connected -> start both server
            init_udp_data_mutex();
            xTaskCreate(udp_server_task, "udp_server_task", STD_STACK_SIZE, udp_server_task_param, STD_PRIO, &udp_server_TaskHandle);
            start_https_server();
        }
    }
}
