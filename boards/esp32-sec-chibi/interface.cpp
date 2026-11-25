#include "CYD28_TouchscreenR.h"
#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <Arduino.h>
#include <interface.h>

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

static bool chibiTouchReady = false;

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
    chibiTouchReady = hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI);
    if (!chibiTouchReady) {
        Serial.println("[TOUCH] XPT2046 not started");
    } else {
        // Require a stronger measured pressure to reject spurious ADC pulses.
        touch.setThreshold(600);
        Serial.println("[TOUCH] XPT2046 started; IRQ-gated and debounced");
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
    // XPT2046 PENIRQ is active LOW. A single physical press may last
    // hundreds of milliseconds: never translate it to repeated menu clicks.
    // Also ignore floating/invalid samples when the input is not asserted.
    static bool pressConsumed = false;
    static uint32_t pressStarted = 0;
    static uint32_t releasedAt = 0;
    static uint8_t stableReleases = 0;
    if (!chibiTouchReady) return;
    const uint32_t now = millis();
    if (digitalRead(XPT2046_TOUCH_CONFIG_INT_GPIO_NUM) != LOW) {
        if (++stableReleases >= 2) {
            if (pressConsumed) releasedAt = now;
            pressConsumed = false;
            pressStarted = 0;
        }
        return;
    }
    stableReleases = 0;
    if (pressConsumed || now - releasedAt < 120) return;
    if (!pressStarted) {
        pressStarted = now;
        return;
    }
    if (now - pressStarted < 35) return;
    BruceTouchPoint first;
    if (!hal_touch_read(touchCfg(), first)) return;
    // A second independent read rejects one-off spikes on shared LCD SPI.
    delay(12);
    if (digitalRead(XPT2046_TOUCH_CONFIG_INT_GPIO_NUM) != LOW) return;
    BruceTouchPoint second;
    if (!hal_touch_read(touchCfg(), second)) return;
    const int dx = abs(int(first.x) - int(second.x));
    const int dy = abs(int(first.y) - int(second.y));
    if (dx > 18 || dy > 18) return;
    if (second.x < 0 || second.y < 0 ||
        second.x >= tft.width() || second.y >= tft.height()) return;
    pressConsumed = true;
    hal_touch_apply(second);
}

void powerOff() { esp_deep_sleep_start(); }

void goToDeepSleep() { esp_deep_sleep_start(); }

void checkReboot() {}
