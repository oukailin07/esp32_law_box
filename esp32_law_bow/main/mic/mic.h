#ifndef MIC_H
#define MIC_H
// I2S configuration
#define I2S_SAMPLE_RATE   (44100)  // 采样率44.1kHz
#define I2S_NUM           (0)      // 使用I2S0
#define I2S_BCK_PIN       (6)     // I2S BCK引脚
#define I2S_WS_PIN        (7)     // I2S WS引脚
#define I2S_DATA_PIN      (5)     // I2S DATA引脚
#define I2S_BUF_LEN       (1024)   // I2S缓冲区长度


void i2s_init();
#endif