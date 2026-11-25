#include "CYD28_TouchscreenR.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <interface.h>

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

// The XPT2046 is mounted on the Chibi panel in portrait; Bruce renders its UI
// in landscape (rotation 1). These transforms are the starting point — run
// Config > Calibrate Touch on the device and save the result to NVS if the
// touch points do not land where you press.
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // rotation:       0      1      2      3
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, false, false, true};
    const bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp — first hook, runs before display and filesystem.
**
** Only the display/touch pins are hardware-verified (see .ini). Every other
** assignment below is a DEFAULT for an external module; the user can remap
** all of them from Config > Set Device pins, which then wins on later boots.
** GPIO33-37 are reserved by the N16R8 octal PSRAM and are never used.
***************************************************************************************/
void _setup_gpio() {
    bruceConfigPins.rotation = 1;

    // I2C (Grove-style header, assumed). ST25R3916/PN532-I2C go here.
    bruceConfigPins.i2c_bus = {(gpio_num_t)4, (gpio_num_t)5};  // sda, scl
    bruceConfigPins.sys_i2c = {(gpio_num_t)4, (gpio_num_t)5};  // sda, scl

    // UART / GPS / CH9329 BadUSB fallback (assumed). On ESP32-S3 the native
    // USB-OTG is used for BadUSB; these are only for external UART adapters.
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43};    // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};     // rx, tx
    bruceConfigPins.badusb_bus = {(gpio_num_t)1, (gpio_num_t)2};    // rx, tx

    // One-pin IR and 433 MHz RF: not wired on the stock board.
    bruceConfigPins.irTx = -1;
    bruceConfigPins.irRx = -1;
    bruceConfigPins.rfTx = -1;
    bruceConfigPins.rfRx = -1;

    // Shared display SPI bus: SCLK=12, MISO=8, MOSI=11. Chip selects below
    // deliberately avoid the LCD CS (10) and touch CS (9).
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6};
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6};
    bruceConfigPins.ST25R_bus = {(gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6};

    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)6, (gpio_num_t)15, GPIO_NUM_NC
    }; // sck, miso, mosi, cs, gdo0, gdo2

    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)16, (gpio_num_t)17
    }; // sck, miso, mosi, cs(ss), ce

    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)21
    }; // sck, miso, mosi, cs

#if !defined(LITE_VERSION)
    bruceConfigPins.W5500_bus = {
        (gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)18, (gpio_num_t)1, (gpio_num_t)2
    }; // sck, miso, mosi, cs, int, rst
    bruceConfigPins.LoRa_bus = {
        (gpio_num_t)12, (gpio_num_t)8, (gpio_num_t)11, (gpio_num_t)38, (gpio_num_t)39, (gpio_num_t)40
    }; // sck, miso, mosi, cs, irq, rst
#endif

    // No speaker, microphone, buzzer or battery gauge are fitted.
    bruceConfigPins.buzzer = -1;

    // Keep the XPT2046 deselected until a conversion starts.
    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);

    // The Chibi panel does not need the ST7789 inversion command.
    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Runs after tft.init() / storage mount. Touch can now share the display SPI.
***************************************************************************************/
void _post_setup_gpio() {
    // Reuse the display's SPI instance: both sides take the SPI transaction lock.
    if (!hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI)) {
        Serial.println("[TOUCH] XPT2046 not started");
    } else {
        Serial.println("[TOUCH] XPT2046 started OK");
    }
    // No backlight GPIO is driven: the Chibi backlight is hardware-controlled.
}

/***************************************************************************************
** Function: _setBrightness — no-op on this board (no confirmed backlight pin).
***************************************************************************************/
void _setBrightness(uint8_t brightval) { (void)brightval; }

/***************************************************************************************
** Function: InputHandler — touch only. The input task handles power-save timing.
***************************************************************************************/
void InputHandler(void) {
    static unsigned long lastTouch = 0;
    if (millis() - lastTouch > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            lastTouch = millis();
            hal_touch_apply(t);
        }
    }
}

void powerOff() { esp_deep_sleep_start(); }

void goToDeepSleep() { esp_deep_sleep_start(); }

void checkReboot() {}
