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
    pinMode(SLP_PIN, OUTPUT);
    digitalWrite(SLP_PIN, HIGH);
    PT_DEVICE->set_kline_led(true);

    pinMode(RX_PIN, INPUT);
    digitalWrite(RX_PIN, HIGH);
    #ifdef ARDUINO_SAM_DUE
          g_APinDescription[RX_PIN].pPort -> PIO_PDR = g_APinDescription[RX_PIN].ulPin;
          g_APinDescription[TX_PIN].pPort -> PIO_PDR = g_APinDescription[TX_PIN].ulPin;
    #endif
    this->obdSerial->begin(baud);

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
    uint8_t len = buf_len;
    uint8_t* new_buf = nullptr;
    if (do_checksum) {
        new_buf = (uint8_t*)malloc(buf_len+1);
        memcpy(&new_buf[0], &buf[0], buf_len);
        new_buf[buf_len] = calc_cs(buf, buf_len);
        len += 1;
    } else {
        new_buf = (uint8_t*)malloc(buf_len);
        memcpy(&new_buf[0], &buf[0], buf_len);
    }

    for (int i = 0; i < len; i++) {
        this->obdSerial->write(new_buf[i]);
        delay(this->p4_min);
    }
    this->obdSerial->setTimeout(this->p1_max*len);
    this->obdSerial->readBytes(new_buf, len);
    delete[] new_buf;
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
        /**
         * Request according the passthru API looks like this
         * request[0] - Format(functional addressing) (First header byte)
         * request[1] - Initialization address used to activate all ECUs (Second header byte)
         * request[2] - Scan tool physical source address (Third header byte)
         * request[3] - Start communication request Service ID (First data byte)
         */
        this->write_data(request, request_len, true); // Todo handle no checksum
        
        /**
         * Some responses I got in W215...nothing  makes sense
         * NOTE: RX fields ignore the echoed back data from TX, that is handled in 'write_data'
         * 
         * TX: 81, 20, F3, 81
         * RX: 73, FF, 81, 15, 90, 5D, 07, 20, 00, 00, 00, 02, F1, 40, 08, 00, 01, 00, 00, 00, 01, 00, 00, 00, 01, 01, 00, 00, 
         *     C0, 32, 07, 20, E8, 09, 07, 20, AD, 15, 08, 00, 04, 00, 00, 00, C9, 32, 07, 20, 01, 00, 00, 00, F7, 08
         * 
         * TX: 81, 20, F3, 81
         * RX: FF, F3, FF, FF, 81, 15, FF, FF, 00, 00, 00, 02, F1, 40, 08, 00, 01, 00, 00, 00, 01, 00, 00, 00, 01, 01, 00, 00, 
         *     C0, 32, 07, 20, E8, 09, 07, 20, AD, 15, 08, 00, 04, 00, 00, 00, C9, 32, 07, 20, 01, 00, 00, 00, F7, 08, 08, 00, 
         *     11, 00, 00, 00, C0, 32, 07, 20, 00, 00
         * 
         * TX: 81, 20, F3, 81
         * RX: 15, FF, FF, 00, 90, 5D, 07, 20, 00, 00, 00, 02, F1, 40, 08, 00, 01, 00, 00, 00, 01, 00, 00, 00
         * 
         * Returning "RX" back to Vediamo results in it crashing 
         */

        this->obdSerial->setTimeout(this->p1_max + this->p3_min);
        uint8_t resp[30];
        if(!this->obdSerial->readBytes(resp, 1)) {
            PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_TIMEOUT, "ECU TIMEOUT");
        }
        uint8_t len = resp[0] & 0b111111;
        uint8_t remainder = len + 2;
        this->obdSerial->setTimeout(this->p1_max * (remainder+1) + this->p3_min);
        if (this->obdSerial->readBytes(&resp[1], remainder)) {
            PCCOMM::respond_ok(MSG_INIT_LIN_CHANNEL, resp, remainder+1);
        } else {
            PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_TIMEOUT, "ECU Timeout");
        }
    }
}

uint8_t Iso9141Channel::calc_cs(uint8_t* buffer, uint8_t len) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < len; i++) {
        res += buffer[i];
    }
    return res;
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