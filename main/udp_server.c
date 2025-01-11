/*******************************************************************
 * File Name         : udp_server.c
 * Description       : Defines UDP Server Task
 *
 * Author            : Noah Plank
 *
 ******************************************************************/

#include "udp_server.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define DOOMSDAY 0

#define PORT CONFIG_UDP_SERVER_PORT

#define DATA_TO_SEND_AS_ANSWER "ACK"
#define LEN_OF_DATA_TO_SEND_AS_ANSWER 3

static const char *TAG = "UDP Server";

SemaphoreHandle_t udp_data_mutex_handle = NULL;

udp_data_t udp_data = {0};

void init_udp_data_mutex()
{
    udp_data_mutex_handle = xSemaphoreCreateMutex();
    if (udp_data_mutex_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create udp_data_mutex!");
    }
}

udp_data_t get_udp_data()
{
    udp_data_t data;
    if (xSemaphoreTake(udp_data_mutex_handle, portMAX_DELAY))
    {
        data = udp_data;
        xSemaphoreGive(udp_data_mutex_handle);
    }
    return data;
}

void udp_server_task(void *param)
{
    char rx_buffer[UDP_BUFFER_SIZE];
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (!DOOMSDAY)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY); // htonl = host to network long (network byte order is big endian)
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol); // SOCK_DGRAM = UDP
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);

        while (1)
        {
            ESP_LOGI(TAG, "Waiting for data");

            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (source_addr.ss_family == PF_INET6)
                {
                    inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);

                xSemaphoreTake(udp_data_mutex_handle, portMAX_DELAY);

                memcpy(&udp_data, rx_buffer, sizeof(udp_data_t));
                ESP_LOGI(TAG, "\nUnix Time: %lld\nNormal Time: %sBattery charge: %d perc.\nweight: %ld g\nmessage: %s\n",
                         udp_data.wakeup_time_in_unix, ctime(&(udp_data.wakeup_time_in_unix)), udp_data.battery_charge_in_perc, udp_data.scale_weight_in_g, udp_data.message_buffer);

                if (udp_data.scale_weight_in_g > 0)
                {
                    *(((udp_server_task_param_t *)param)->mail_available) = true;
                }
                else
                {
                    *(((udp_server_task_param_t *)param)->mail_available) = false;
                }

                xSemaphoreGive(udp_data_mutex_handle);

                ESP_LOGI(TAG, "Sending answer %s", DATA_TO_SEND_AS_ANSWER);
                int err = sendto(sock, DATA_TO_SEND_AS_ANSWER, LEN_OF_DATA_TO_SEND_AS_ANSWER, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}
