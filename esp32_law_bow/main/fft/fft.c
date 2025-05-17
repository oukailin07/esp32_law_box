#include "esp_dsp.h"
#include <math.h>
#include "driver/i2s_std.h"
#include "dsps_fft2r.h"
#include "dsps_math.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

#define TAG "FFT_VIS"
#define NUM_BARS 15
#define FFT_SIZE 1024
#define I2S_BUF_LEN       (1024)   // I2S缓冲区长度
float fft_table[CONFIG_DSP_MAX_FFT_SIZE];
#define MAX_FFT_INPUT_VALUE 32767.0f
#define SMOOTHING_FACTOR 0.3f
static float fft_result[NUM_BARS] = {0};
QueueHandle_t fft_result_queue;
// 采样缓冲区
int16_t *i2s_read_buff; // 16-bit stereo, 2倍空间
__attribute__((aligned(16)))
float fft_input[2 * FFT_SIZE];       // interleaved real/imag
__attribute__((aligned(16)))
float fft_output[FFT_SIZE/2];          // magnitude

typedef struct {
    int bin_start;
    int bin_end;
} fft_band_t;
static const fft_band_t fft_15bands_48k[NUM_BARS] = {
    {1, 1},    // ≈ 20.0 ~ 31.4 Hz
    {2, 2},    // ≈ 31.4 ~ 49.3 Hz
    {3, 4},    // ≈ 49.3 ~ 77.3 Hz
    {5, 6},    // ≈ 77.3 ~ 121.1 Hz
    {7, 10},   // ≈ 121.1 ~ 189.7 Hz
    {11, 15},  // ≈ 189.7 ~ 297.0 Hz
    {16, 23},  // ≈ 297.0 ~ 464.6 Hz
    {24, 35},  // ≈ 464.6 ~ 726.6 Hz
    {36, 53},  // ≈ 726.6 ~ 1136.0 Hz
    {54, 79},  // ≈ 1136.0 ~ 1777.5 Hz
    {80, 118}, // ≈ 1777.5 ~ 2781.4 Hz
    {119, 176},// ≈ 2781.4 ~ 4354.3 Hz
    {177, 260},// ≈ 4354.3 ~ 6817.0 Hz
    {261, 383},// ≈ 6817.0 ~ 10672.0 Hz
    {384, 511} // ≈ 10672.0 ~ 24000.0 Hz
};


extern i2s_chan_handle_t rx_handle;

// 音频采集与 FFT 分析任务
void fft_task(void *arg)
{
    
    // 检查 FFT 输入缓冲区是否初始化
    if (!i2s_read_buff) {
        ESP_LOGE(TAG, "FFT buffers not initialized!");
        vTaskDelete(NULL);  // 删除当前任务
        return;
    }
    // 计算读取缓冲区长度（4 通道，每个通道 16 位，FFT_SIZE 个采样点）
    int buffer_len = sizeof(int16_t) * 4 * FFT_SIZE;
    while (1) {

        size_t bytes_read; // 实际读取到的字节数
        // 从 I2S 接口读取音频数据
        //esp_codec_dev_read(record_dev_handle, (void *)i2s_read_buff, buffer_len);
        ESP_ERROR_CHECK(i2s_channel_read(rx_handle, (void *)i2s_read_buff, I2S_BUF_LEN, &bytes_read, portMAX_DELAY));
        // 预处理采样数据，并准备 FFT 输入
        for (int i = 0; i < FFT_SIZE; i++) {
            int16_t mixed = i2s_read_buff[i];

            // 转换为 float 类型，并限制值范围
            float sample = (float)mixed;
            fft_input[2 * i] = fmaxf(fminf(sample, MAX_FFT_INPUT_VALUE), -MAX_FFT_INPUT_VALUE);  // 实部
            fft_input[2 * i + 1] = 0.0f;  // 虚部设为 0
        }

        // 进行 FFT 计算（实数输入，复数输出）
        dsps_fft2r_fc32(fft_input, FFT_SIZE);

        // 位反转（必要的 FFT 后处理步骤）
        dsps_bit_rev_fc32(fft_input, FFT_SIZE);

        // 计算频谱幅值并进行归一化和平滑处理
        for (int i = 0; i < FFT_SIZE / 2; i++) {
            float real = fft_input[2 * i];     // 实部
            float imag = fft_input[2 * i + 1]; // 虚部
            float mag = sqrtf(real * real + imag * imag);  // 幅值计算
            mag = isnan(mag) || mag < 0.0f ? 0.0f : mag;    // 处理异常值

            // 归一化到 0~100 范围
            float norm_mag = fminf(fmaxf(mag / MAX_FFT_INPUT_VALUE * 100.0f, 0.0f), 100.0f);

            // 使用滑动平均滤波实现频谱平滑效果
            fft_output[i] = (1 - SMOOTHING_FACTOR) * fft_output[i] + SMOOTHING_FACTOR * norm_mag;

            if (isnan(fft_output[i])) {
                fft_output[i] = 0.0f;  // 处理 NaN
            }
        }

        for (int i = 0; i < NUM_BARS; i++) {
            float sum = 0;
            int count = 0;
            for (int j = fft_15bands_48k[i].bin_start; j <= fft_15bands_48k[i].bin_end; j++) {
                sum += fft_output[j];
                count++;
            }
            fft_result[i] = (count > 0) ? (sum / count) : 0.0f;
        }

        // 将频谱结果发送到消息队列中（覆盖旧数据）
        xQueueOverwrite(fft_result_queue, fft_result);

        // 小延时，释放 CPU 给其他任务使用
        vTaskDelay(1);
    }
}


void fft_init()
{
    dsps_fft2r_init_fc32(fft_table, CONFIG_DSP_MAX_FFT_SIZE);
    // 创建消息队列
    fft_result_queue = xQueueCreate(1, sizeof(fft_result));
    if (fft_result_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create FFT result queue");
        return;
    }
    i2s_read_buff = (int16_t *)heap_caps_malloc(sizeof(int16_t)  * FFT_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (i2s_read_buff == NULL) {
        ESP_LOGE(TAG, "Failed to allocate I2S read buffer");
        vTaskDelete(NULL);
        return;
    }
    // 创建 FFT 任务
    xTaskCreatePinnedToCore(fft_task, "FFT_Task", 4096, NULL, 5, NULL,1);
}