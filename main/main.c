/* USB Audio Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "tinyusb.h"
#include "tusb_audio.h"
#include "sig_gen.h"

#define SAMPLES_BYTES_NUM CFG_TUD_AUDIO_EP_SZ_IN
#define SAMPLES_NUM CFG_TUD_AUDIO_EP_SZ_IN / 2

static const char *TAG = "USB audio example";

uint16_t audio_buffer[SAMPLES_BYTES_NUM];

//! Add more channels if needed. Needs to be initialized and packed correctly
sig_gen_t sine_ch1;
sig_gen_t sine_ch2;

void tinyusb_audio_tx_callback(uint8_t *buffer, uint16_t *bytes)
{
    uint16_t ch1_samples[48];
    uint16_t ch2_samples[48];
    // Get samples from signal generator
    sig_gen_output(&sine_ch1, (uint8_t *)ch1_samples, 48);
    sig_gen_output(&sine_ch2, (uint8_t *)ch2_samples, 48);

    // Arrange samples in buffer
    uint16_t *p_buff = audio_buffer;
    for (int samples_num = 0; samples_num < 48; samples_num++)
    {
        *p_buff++ = ch1_samples[samples_num];
        *p_buff++ = ch2_samples[samples_num];
    }

    memcpy(buffer, audio_buffer, SAMPLES_BYTES_NUM);
    *bytes = SAMPLES_BYTES_NUM;
}

void app_main(void)
{
    ESP_LOGI(TAG, "USB initialization");

    // Configure signal generator
    sig_gen_config_t sig_gen_cfg = {
        .gen_source = SINE_LUT,
        .lut_freq = LUT_FREQ_440,
        .bytes_per_sample = CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX,
        .sample_rate = 48000,
        .endianess = SIG_GEN_LE,
        .enable_cb = SIG_GEN_NO_CB};
    sig_gen_init(&sine_ch1, &sig_gen_cfg);
    sig_gen_cfg.lut_freq = LUT_FREQ_552;
    sig_gen_init(&sine_ch2, &sig_gen_cfg);

    // Configure tinyusb audio
    tinyusb_config_audio_t audio_cfg = {
        .audio_tx_callback = &tinyusb_audio_tx_callback};
    tusb_audio_init(&audio_cfg);

    // Install TINYUSB driver
    tinyusb_config_t tusb_cfg = {
        .external_phy = false,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}
