#ifndef PT_DEVICE_H_
#define PT_DEVICE_H_

#include <stdint.h>

class pt_device {
public:
    bool is_connected = false;
    void init_device();

    // LED functions
    void set_rx_led(bool state);
    void set_tx_led(bool state);
    void set_status_led(bool state);
    void set_can_led(bool state);
    void set_kline_led(bool state);
    void set_rgb_led(uint8_t r, uint8_t g, uint8_t b);

    // Getter functions for PT API
    int read_batt_mv();
    const char* get_firmware_version() {
        return this->FIRMWARE_VERSION;
    };
private:
    const char* FIRMWARE_VERSION = "1.0.0";
};

extern pt_device* PT_DEVICE;

#endif