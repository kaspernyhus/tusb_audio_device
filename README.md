| Supported Targets | ESP32-S3 |
| ----------------- | -------- |

Fork of Kasper's tinyUSB audio example.
Main goal is to get I2S audio transfer working and to enable higer audio bitrates.


# TinyUSB Audio Device example

Using a signal generator to output 1/2/4 sine waves over USB Audio to host (USB Microphone)

Change number of audio channels in `menuconfig` ---> `Component config` ---> `ESP TinyUSB Audio`

(NOTE: Only supports 48kHz/16bit audio)

