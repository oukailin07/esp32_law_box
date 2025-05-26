#include "ws2812.h"
#include "esp_log.h"
static const char *TAG = "LED";
led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}


int xy_to_index(int x, int y, int width) {
    if (y % 2 == 0) {
        // 偶数行，从左到右
        return y * width + x;
    } else {
        // 奇数行，从右到左
        return y * width + (width - 1 - x);
    }
}
// 每个数字为 5x3 点阵（高 x 宽）
const uint8_t font_3x5[11][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
    {0b000, 0b010, 0b000, 0b010, 0b000}, // ':' 冒号
};

void draw_char(led_strip_handle_t strip, int char_index, int x_offset, int y_offset) {
    for (int y = 0; y < 5; y++) {
        uint8_t row = font_3x5[char_index][y];
        for (int x = 0; x < 3; x++) {
            bool pixel_on = (row >> (2 - x)) & 0x01;
            int led_x = x + x_offset;
            int led_y = y + y_offset;
            if (led_x < MATRIX_WIDTH && led_y < MATRIX_HEIGHT) {
                int index = xy_to_index(led_x, led_y, MATRIX_WIDTH);
                if (pixel_on) {
                    led_strip_set_pixel(strip, index, 0, 255, 0);  // 绿色
                } else {
                    led_strip_set_pixel(strip, index, 0, 0, 0);
                }
            }
        }
    }
}

void draw_time(led_strip_handle_t strip, int hour, int min) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, min);
    int x = 0;
    for (int i = 0; i < 5; i++) {
        int ch = (buffer[i] == ':') ? 10 : (buffer[i] - '0');
        draw_char(strip, ch, x, 1);  // 从 y=1 开始居中显示
        x += 4; // 每字符宽3 + 1列空隙
    }
    led_strip_refresh(strip);
}