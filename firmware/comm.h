#ifndef COMM_H_
#define COMM_H_

#include <stdint.h>
#include <Arduino.h>
#include "MACCHINA_CONFIG.h"
#include "pt_device.h"


#define MSG_UNKNOWN 0x00
#define MSG_LOG 0x01
#define MSG_OPEN_CHANNEL 0x02  //[ID, Baud, Baud, Protocol]
#define MSG_CLOSE_CHANNEL 0x03
#define MSG_SET_CHAN_FILT 0x04  //[ID, Baud, Baud, Protocol]
#define MSG_REM_CHAN_FILT 0x05
#define MSG_TX_CHAN_DATA 0x06
#define MSG_RX_CHAN_DATA 0x07
#define MSG_READ_BATT 0x08
#define MSG_IOCTL_SET 0x09
#define MSG_IOCTL_GET 0x10
#define MSG_STATUS 0xAA // Args: [0] -> 0x00 = Goodbye, 0x01 = Hellow
#define MSG_GET_FW_VERSION 0xAB
#define MSG_TEST 0xFF

// Reserve 4Kb of memory for a temp buffer for reading and writing comm messages. Basically, a larger serial buffer
#define COMM_MSG_SIZE 8192
#define COMM_MSG_ARG_SIZE COMM_MSG_SIZE-4

struct __attribute__ ((packed)) COMM_MSG {
    uint8_t msg_id;
    uint8_t msg_type;
    uint16_t arg_size;
    uint8_t args[COMM_MSG_ARG_SIZE];
};

namespace PCCOMM {
    
    void init();

    bool read_message(COMM_MSG *msg);

    void send_message(COMM_MSG *msg);

    void log_message(char* msg);

    void respond_ok(uint8_t op, uint8_t* args, uint16_t arg_size);

    void respond_err(uint8_t op, uint8_t error_id, char* txt);

    void send_rx_data(uint8_t channel_id, uint32_t rx_status, char* data, uint16_t data_len);

    void reset();

    uint8_t get_last_id();
}

#endif