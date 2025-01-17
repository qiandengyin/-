/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "app_wifi.h"
#include "app_udp_client.h"

static const char *TAG = "UPD CLIENT";

#define CONFIG_EXAMPLE_IPV4      1
#define CONFIG_EXAMPLE_IPV4_ADDR "255.255.255.255"
#define CONFIG_EXAMPLE_PORT      3333

#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR ""
#endif

static int sg_addr_family = 0;
static int sg_ip_protocol = 0;
static int sg_sock = -1;
#if defined(CONFIG_EXAMPLE_IPV4)
static struct sockaddr_in  sg_dest_addr = {0};
#elif defined(CONFIG_EXAMPLE_IPV6)
static struct sockaddr_in6 sg_dest_addr = {0};
#endif

/// @brief 创建 UDP 客户端套接字
/// @param
static void app_udp_client_create_socket(void)
{
    if (sg_sock != -1)
        return;

#if defined(CONFIG_EXAMPLE_IPV4)
    sg_dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    sg_dest_addr.sin_family = AF_INET;
    sg_dest_addr.sin_port = htons(CONFIG_EXAMPLE_PORT);
    sg_addr_family = AF_INET;
    sg_ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
    inet6_aton(HOST_IP_ADDR, &sg_dest_addr.sin6_addr);
    sg_dest_addr.sin6_family = AF_INET6;
    sg_dest_addr.sin6_port = htons(CONFIG_EXAMPLE_PORT);
    sg_dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
    sg_addr_family = AF_INET6;
    sg_ip_protocol = IPPROTO_IPV6;
#endif
    sg_sock = socket(sg_addr_family, SOCK_DGRAM, sg_ip_protocol);
    if (sg_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sg_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, CONFIG_EXAMPLE_PORT);
}

/// @brief udp client 发送数据
/// @param data
/// @param len
void app_udp_client_send_data(uint8_t *data, int len)
{
    if (app_wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
        return;

    app_udp_client_create_socket();
    if (sg_sock == -1)
        return;

    if (len > 8)
        return;
    
    // 按键状态改变, 发送数据
    uint8_t key_changed = 0;
    static uint8_t key_buffer_last[8] = {0};
    uint8_t key_buffer[8] = {0};
    memcpy(key_buffer, data, len);
    for (uint8_t i = 0; i < 8; i++)
    {
        if (key_buffer[i] != key_buffer_last[i])
            key_changed++;
    }
    if (key_changed == 0)
        return;
    memcpy(key_buffer_last, key_buffer, sizeof(key_buffer));

    app_wifi_lock(0);
    int err = sendto(sg_sock, (const uint8_t *)key_buffer, sizeof(key_buffer), 0, (struct sockaddr *)&sg_dest_addr, sizeof(sg_dest_addr));
    app_wifi_unlock();
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        if (sg_sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sg_sock, 0);
            close(sg_sock);
            sg_sock = -1;
        }
    }
}