#include "MACCHINA_CONFIG.h"
#if defined(CFG_MACCHINA_M2)

#include "pt_device.h"
#include <M2_12VIO.h>


pt_device* PT_DEVICE = new pt_device();

M2_12VIO M2IO;

void pt_device::init_device() {
    SerialUSB.begin(500000); // 500kbps for M2
    pinMode(DS6, OUTPUT); // Green
    pinMode(DS5, OUTPUT); // Yellow
    pinMode(DS4, OUTPUT); // Yellow
    pinMode(DS3, OUTPUT); // Yellow
    pinMode(DS2, OUTPUT); // Red
    pinMode(DS7_GREEN, OUTPUT); // RGB (Green)
    pinMode(DS7_BLUE, OUTPUT);  // RGB (Blue)
    pinMode(DS7_RED, OUTPUT);   // RGB (Red)
    digitalWrite(DS2, HIGH);
    digitalWrite(DS6, HIGH);
    digitalWrite(DS5, HIGH);
    digitalWrite(DS4, HIGH);
    digitalWrite(DS3, HIGH);
    digitalWrite(DS7_GREEN, HIGH);
    digitalWrite(DS7_BLUE, HIGH);
    digitalWrite(DS7_RED, HIGH);
    M2IO.Init_12VIO();
    this->set_status_led(false); // Assume no connection on statup!
}

int pt_device::read_batt_mv() {
    int voltage=M2IO.Supply_Volts() / 100;
    if (voltage <= 62) { // Assume no voltage
        return 0;
    }
    return voltage * 100;
}

void pt_device::set_status_led(bool state) {
    if (state) {
        digitalWrite(DS6, LOW);
        digitalWrite(DS2, HIGH);
        this->is_connected = true;
    } else {
        digitalWrite(DS6, HIGH);
        digitalWrite(DS2, LOW);
        this->is_connected = false;
    }
}


void pt_device::set_rx_led(bool state){
    if (state) {
        digitalWrite(DS7_BLUE, LOW);
    } else {
        digitalWrite(DS7_BLUE, HIGH);
    }
}
void pt_device::set_tx_led(bool state){
    if (state) {
        digitalWrite(DS7_GREEN, LOW);
    } else {
        digitalWrite(DS7_GREEN, HIGH);
    }
}
void pt_device::set_can_led(bool state){
    if (state) {
        digitalWrite(DS3, LOW);
    } else {
        digitalWrite(DS3, HIGH);
    }
}
void pt_device::set_kline_led(bool state){
    if (state) {
        digitalWrite(DS4, LOW);
    } else {
        digitalWrite(DS4, HIGH);
    }
}

#endif

