#include <stdio.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
 
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
 
#include <netdb.h>
#include <sys/socket.h>
 
#include "WIFI/ConnectWIFI.h"
#include "my_dns_server.h"
 
#include "webserver.h"
 
#include "ws2812.h"

static const char *TAG = "main";
int spectrum[MATRIX_WIDTH];  // 16列频谱高度


#include "esp_sntp.h"

void initialize_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

bool obtain_time() {
    initialize_sntp();
    for (int retry = 0; retry < 10; ++retry) {
        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year > (2020 - 1900)) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    return false;
}

void app_main(void)
{
    led_strip_handle_t led_strip = configure_led();
    bool led_on_off = false;

    ESP_LOGI(TAG, "Start blinking LED strip");
    while (1) {
        if (led_on_off) {
            /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
            for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 5, 5, 5));
            }
            /* Refresh the strip to send data */
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            ESP_LOGI(TAG, "LED ON!");
        } else {
            /* Set all LED off to clear all pixels */
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            ESP_LOGI(TAG, "LED OFF!");
        }

        led_on_off = !led_on_off;
        vTaskDelay(pdMS_TO_TICKS(500));
    }


    while (1) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            spectrum[x] = rand() % (MATRIX_HEIGHT + 1);  // 随机高度
        }
        // 绘制频谱图
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            for (int y = 0; y < MATRIX_HEIGHT; y++) {
                int index = xy_to_index(x, y, MATRIX_WIDTH);
                if (y < spectrum[x]) {
                    led_strip_set_pixel(led_strip, index, 0, 100, 255); // 蓝绿色
                } else {
                    led_strip_set_pixel(led_strip, index, 0, 0, 0); // 黑
                }
            }
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

        initialize_sntp();
        if (obtain_time()) {
        while (1) {
            time_t now;
            time(&now);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);

            draw_time(led_strip, timeinfo.tm_hour, timeinfo.tm_min);
            vTaskDelay(pdMS_TO_TICKS(60000));  // 每分钟刷新一次
        }
    }
}

// void app_main(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     NvsWriteDataToFlash("","","");
//     char WIFI_Name[50] = { 0 };
//     char WIFI_PassWord[50] = { 0 }; /*读取保存的WIFI信息*/
//     if(NvsReadDataFromFlash("WIFI Config Is OK!",WIFI_Name,WIFI_PassWord) == 0x00)
//     {
//         printf("WIFI SSID     :%s\r\n",WIFI_Name    );
//         printf("WIFI PASSWORD :%s\r\n",WIFI_PassWord);
//         printf("开始初始化WIFI Station 模式\r\n");
//         wifi_init_sta(WIFI_Name,WIFI_PassWord);         /*按照读取的信息初始化WIFI Station模式*/
        
//     }
//     else
//     {
//         printf("未读取到WIFI配置信息\r\n");
//         printf("开始初始化WIFI AP 模式\r\n");
//         wifi_init_softap_sta();
//             //WIFI_AP_Init();     /*上电后配置WIFI为AP模式*/
//         //vTaskDelay(1000 / portTICK_PERIOD_MS);
//         dns_server_start();  //开启DNS服务
//         web_server_start();  //开启http服务
//     }
 
// }