#include "comm_channels.h"
#include "pt_device.h"

#if defined(CFG_MACCHINA_M2)

#define RX_PIN LIN_KRX
#define TX_PIN LIN_KTX
#define SLP_PIN LIN_KSLP

void Iso9141Channel::set_port(bool state) {
    if (state) {
        this->obdSerial->begin(this->baud);
    } else {
        this->obdSerial->end();
        pinMode(SLP_PIN, OUTPUT);
        digitalWrite(SLP_PIN, HIGH);
    }
}

void Iso9141Channel::set_line(bool state) {
    digitalWrite(TX_PIN, state);
}


bool Iso9141Channel::setup(int id, int protocol, int baud, int flags) {
    this->channel_id = id;
    this->obdSerial = &Serial1;
    this->baud = baud;
    this->obdSerial->begin(baud);
    pinMode(SLP_PIN, OUTPUT);
    digitalWrite(SLP_PIN, HIGH);
    PT_DEVICE->set_kline_led(true);
    pinMode(RX_PIN, INPUT_PULLUP);

    // Default timings
    uint32_t p1_min = 0;
    uint32_t p1_max = 20;
    uint32_t p2_min = 25;
    uint32_t p2_max = 50;
    uint32_t p3_min = 55;
    uint32_t p3_max = 5000;
    uint32_t p4_min = 5;
    uint32_t p4_max = 20;

    uint32_t w1 = 300;
    uint32_t w2 = 20;
    uint32_t w3 = 20;
    uint32_t w4 = 50;
    uint32_t w5 = 300;

    uint32_t tidle = w5;
    uint32_t tinl = 25;
    uint32_t twup = 50;

    uint32_t parity = 0;
    return true;
}

void Iso9141Channel::addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len) {
    PCCOMM::respond_ok(MSG_SET_CHAN_FILT, nullptr, 0);
}

void Iso9141Channel::update() {
    
}

void Iso9141Channel::removeFilter(int id) {
    PCCOMM::respond_ok(MSG_REM_CHAN_FILT, nullptr, 0);
}

void Iso9141Channel::destroy() {
    this->obdSerial->end();
    this->obdSerial = nullptr;
    PT_DEVICE->set_kline_led(false);
}

/**
 * Macchina will NOT respond to this request, just send and leave it
 */
void Iso9141Channel::sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond) {
    PCCOMM::respond_ok(MSG_TX_CHAN_DATA, nullptr, 0);
}

void Iso9141Channel::write_data(uint8_t* buf, uint8_t buf_len, bool do_checksum) {
    if (do_checksum) {
        uint8_t* new_buf = (uint8_t*)malloc(buf_len+1);
        memcpy(&new_buf[0], &buf[0], buf_len);
        this->write_cs(new_buf, buf_len);
        this->obdSerial->write(new_buf, buf_len+1);
        delete[] new_buf;
    } else {
        this->obdSerial->write(buf, buf_len);
    }
}


void Iso9141Channel::ioctl_get(uint32_t id) {
    PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "ISO9141 IOCTL get unimplemented");
}

void Iso9141Channel::wakeup(uint8_t type, uint8_t* request, uint8_t request_len) {
    PCCOMM::log_message("Wakeup started");
    if (type == 0) {
        // TODO FIVE BAUD INIT
        PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_FAILED, "Five baud init TODO");
    } else {
        // Fast init
        this->set_port(false);
        this->set_line(true);
        delay(this->tidle);
        this->set_line(false); delay(this->tinl);
        this->set_line(true); delay(this->twup);

        this->set_port(true);

        this->write_data(request, request_len, true); // Todo handle no checksum
        this->obdSerial->setTimeout(this->p1_max);
        
        uint8_t buf[10];
        if (this->obdSerial->readBytes(buf, 1)) {
            char sbuf[5];
            sprintf(sbuf, "%d", buf[0]);
            PCCOMM::log_message(sbuf);
        } else {
            PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_FAILED, "Err TIMEOUT");
        }

        
        PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_FAILED, "WIP TODO");

    }
}

void Iso9141Channel::write_cs(uint8_t* buffer, uint8_t len) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < len; i++) {
        res += buffer[i];
    }
    buffer[len] = res;
}

void Iso9141Channel::ioctl_set(uint32_t id, uint32_t value) {
    if (id == P1_MIN) {
        this->p1_min = value;
    } else if (id == P1_MAX) {
        this->p1_max = value;
    } else if (id == P2_MIN) {
        this->p2_min = value;
    } else if (id == P2_MAX) {
        this->p2_max = value;
    } else if (id == P3_MIN) {
        this->p3_min = value;
    } else if (id == P3_MAX) {
        this->p3_max = value;
    } else if (id == P4_MIN) {
        this->p4_min = value;
    } else if (id == P4_MAX) {
        this->p4_max = value;
    } else if (id == W1) {
        this->w1 = value;
    } else if (id == W2) {
        this->w2 = value;
    } else if (id == W3) {
        this->w3 = value;
    } else if (id == W4) {
        this->w4 = value;
    } else if (id == W5) {
        this->w5 = value;
    } else if (id == TIDLE) {
        this->tidle = value;
    } else if (id == TINL) {
        this->tinl = value;
    } else if (id == TWUP) {
        this->twup = value;
    } else if (id == PARITY) {
        this->parity = value;
    } else {
        PCCOMM::respond_err(MSG_IOCTL_SET, ERR_FAILED, "ISO9141 IOCTL set unimplemented");
        return;
    }
    PCCOMM::respond_ok(MSG_IOCTL_SET, nullptr, 0);
}

#endif