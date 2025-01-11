#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "stdbool.h"
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct
{
    time_t wakeup_time_in_unix;
    uint8_t battery_charge_in_perc;
    uint32_t scale_weight_in_g;
    char message_buffer[128];
} udp_data_t;

#define UDP_BUFFER_SIZE (sizeof(udp_data_t))

typedef struct
{
    bool *mail_available;
} udp_server_task_param_t;

udp_data_t get_udp_data();
void init_udp_data_mutex();
void udp_server_task(void *);

#endif