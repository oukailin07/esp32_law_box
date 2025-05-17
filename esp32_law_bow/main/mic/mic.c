#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2s_std.h" // 使用新的I2S标准模式API
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mic.h"


// I2S通道和GPIO配置
i2s_chan_handle_t rx_handle; // I2S接收通道句柄

// Function to initialize I2S
void i2s_init() {
    // I2S通道配置
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle)); // 只创建接收通道

    // I2S标准模式配置
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // MCLK未使用
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED, // 未使用（输出引脚）
            .din = I2S_DATA_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 初始化I2S通道
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    // 启用I2S通道
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}