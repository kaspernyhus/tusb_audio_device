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
#include "driver/i2s_std.h"
#include "sig_gen.h"
#include "driver/gpio.h"

#define SAMPLES_BYTES_NUM CFG_TUD_AUDIO_EP_SZ_IN
#define SAMPLES_NUM CFG_TUD_AUDIO_EP_SZ_IN / 2

static const char *TAG = "USB audio example";

uint16_t audio_buffer[SAMPLES_BYTES_NUM];

//! Add more channels if needed. Needs to be initialized and packed correctly
sig_gen_t sine_ch1;
sig_gen_t sine_ch2;


#define EXAMPLE_STD_BCLK_IO1 GPIO_NUM_7  // I2S bit clock io number
#define EXAMPLE_STD_WS_IO1 GPIO_NUM_46   // I2S word select io number
#define EXAMPLE_STD_DOUT_IO1 GPIO_NUM_10 // I2S data out io number
#define EXAMPLE_STD_DIN_IO1 GPIO_NUM_5   // I2S data in io number


#define EXAMPLE_BUFF_SIZE (48*8)  //main buffer size used between i2s and usb 

uint8_t  *buffer0;  //buffer pointer

static i2s_chan_handle_t tx_chan; // I2S tx channel handler
static i2s_chan_handle_t rx_chan; // I2S rx channel handler


void init_buffers(){
    buffer0 = (uint8_t *)calloc(1, EXAMPLE_BUFF_SIZE);
    assert(buffer0);
}

static void i2s_example_read_task(void *args)
{
    uint8_t * r_buf;
    size_t r_bytes = 0;
    while (1)
    {
        r_buf = buffer0;
        //printf("%p",r_buf);
        gpio_set_level(GPIO_NUM_12, 1);
        /* Read i2s data */
        if (i2s_channel_read(rx_chan, r_buf, EXAMPLE_BUFF_SIZE, &r_bytes, 1000) == ESP_OK){
            gpio_set_level(GPIO_NUM_12, 0);
        }
        else{
            printf("Read Task: i2s read failed\n");
        }
        
    }
    free(r_buf);
    vTaskDelete(NULL);
}



static void i2s_example_init_std_duplex(void){
    
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_SLAVE);
    chan_cfg.dma_frame_num = EXAMPLE_BUFF_SIZE/8;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan));

  
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // some codecs may require mclk signal, this example doesn't need it
            .bclk = EXAMPLE_STD_BCLK_IO1,
            .ws = EXAMPLE_STD_WS_IO1,
            .dout = EXAMPLE_STD_DOUT_IO1,
            .din = EXAMPLE_STD_DIN_IO1, // In duplex mode, bind output and input to a same gpio can loopback internally
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    /* Initialize the channels */
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
}




void tinyusb_audio_tx_callback(uint8_t *buffer, uint16_t *bytes){   
    gpio_set_level(GPIO_NUM_14, 1);
    uint8_t* pointer8 = buffer0;
    
    gpio_set_level(GPIO_NUM_14, 0);

    // Arrange samples in buffer
    uint16_t *p_buff = audio_buffer;
    uint16_t *pointer16 = (uint16_t *)pointer8;
    for (int samples_num = 0; samples_num < 48; samples_num+=1)
    {
        //gpio_set_level(GPIO_NUM_17, 1);
        *p_buff++ = *(pointer16+((samples_num*4)+1));
        *p_buff++ = *(pointer16+((samples_num*4)+3));
        //gpio_set_level(GPIO_NUM_17, 0);
    }
    gpio_set_level(GPIO_NUM_14, 1);
    memcpy(buffer, audio_buffer, SAMPLES_BYTES_NUM);
    *bytes = SAMPLES_BYTES_NUM;
    gpio_set_level(GPIO_NUM_14, 0);
}




void app_main(void)
{
    ESP_LOGI(TAG, "USB initialization");
    init_buffers();

    //GPIO pins for debugging via logic analyser
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_12, 1);

    i2s_example_init_std_duplex();
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

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
    xTaskCreate(i2s_example_read_task, "i2s_example_read_task", 4096, NULL, 5, NULL);
}
