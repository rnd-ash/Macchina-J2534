#include "comm.h"
#include <HardwareSerial.h>

#if defined(CFG_MACCHINA_M2)



namespace PCCOMM {
    uint8_t last_id = 0;
    char* tempbuf;
    uint16_t read_count = 0;
    bool isReadingMsg = false;
    uint16_t read_target = 0;
    bool read_message(COMM_MSG *msg) {
        if (!isReadingMsg && SerialUSB.available() >= 2) { // Starter of payload
            PT_DEVICE->set_rx_led(true);
            isReadingMsg = true;
            read_count = 0;
            SerialUSB.readBytes((char*)&read_target, 2);
            tempbuf = new char[read_target];
            return false;
        } else if(isReadingMsg && SerialUSB.available() > 0) { // Just reading data
            // Calculate how many bytes to read (min of avaliable bytes, or left to read to complete the data)
            uint16_t maxRead = min(SerialUSB.available(), read_target-read_count);
            SerialUSB.readBytes(&tempbuf[read_count], maxRead);
            read_count += maxRead;

            // Size OK now, full payload received
            if(read_count == read_target) {
                msg->arg_size = read_target - 2;
                isReadingMsg = false;
                msg->msg_id = tempbuf[0];
                msg->msg_type = tempbuf[1];
                memcpy(msg->args, &tempbuf[2], msg->arg_size);
                if (msg->msg_id != 0x00) {
                    last_id = msg->msg_id;
                }
                delete[] tempbuf;
                PT_DEVICE->set_rx_led(false);
                return true;
            }
        }
        return false;
    }

    void init(){}; // This does nothing on M2

    void send_message(COMM_MSG *msg) {
        PT_DEVICE->set_tx_led(true);
        SerialUSB.write((uint8_t*)msg, sizeof(COMM_MSG));
        SerialUSB.flush(); // Wait for IO to complete!
        PT_DEVICE->set_tx_led(false);
    }

    // This is used for log_message, respond_ok and respond_err
    COMM_MSG res = {0x00};

    void log_message(char* msg) {
        memset(&res, 0x00, sizeof(COMM_MSG));
        res.msg_type = MSG_LOG;
        res.arg_size = min((int)strlen(msg), COMM_MSG_ARG_SIZE);
        memcpy(&res.args[0], msg, res.arg_size);
        send_message(&res);
    }

    void respond_ok(uint8_t op, uint8_t* args, uint16_t arg_size) {
        memset(&res, 0x00, sizeof(COMM_MSG));
        res.msg_type = op;
        res.arg_size = 1 + min((int)arg_size, COMM_MSG_ARG_SIZE);
        res.msg_id = last_id;
        res.args[0] = 0x00; // STATUS_NOERROR
        if (arg_size != 0) {
            memcpy(&res.args[1], args, res.arg_size-1);
        }
        send_message(&res);
    }

    void respond_err(uint8_t op, uint8_t error_id, char* txt) {
        memset(&res, 0x00, sizeof(COMM_MSG));
        res.msg_type = op;
        res.arg_size = 1 + min((int)strlen(txt), COMM_MSG_ARG_SIZE);
        res.args[0] = error_id;
        res.msg_id = last_id;
        memcpy(&res.args[1], txt, res.arg_size-1);
        send_message(&res);
    }

    void send_rx_data(uint8_t channel_id, uint32_t rx_status, char* data, uint16_t data_len) {
        memset(&res, 0x00, sizeof(COMM_MSG));
        res.msg_type = MSG_RX_CHAN_DATA;
        res.arg_size = 5 + min((int)data_len, COMM_MSG_ARG_SIZE);
        res.args[0] = channel_id;
        res.msg_id = 0x00;
        memcpy(&res.args[1], &rx_status, 4);
        memcpy(&res.args[5], data, res.arg_size-5);
        send_message(&res);
    }

    /**
     * Called on M2 disconnect
     */
    void reset() {
        isReadingMsg = false;
        read_count = 0;
        read_target = 0;
        last_id = 0;
        if (isReadingMsg) {
            delete[] tempbuf;
            tempbuf = nullptr;
            isReadingMsg = false;
        }
        // Empty any remaining serial data
        while (Serial.available()) {
            Serial.read();
        }
    }
}

#endif