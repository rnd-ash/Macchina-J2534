#include "comm.h"

#if defined(CFG_MACCHINA_A0) || defined(CFG_MACCHINA_ESP32_TEST)
#include <driver/uart.h>

#define SERIAL_RX_SIZE COMM_MSG_SIZE

COMM_MSG tx_tmp;
bool send_change = false;


COMM_MSG rx_tmp;
bool receive_change = false;

uint8_t tmp_queue[4096];
uint16_t read_pos = 0;
uint16_t target_read = 0;
bool is_reading = false;
void loop_rx(void *pvParameters) {
    size_t avaliable;
    while(true) {
        uart_get_buffered_data_len(UART_NUM_0, &avaliable);
        if (avaliable > 2 && !is_reading) {
            // Payload begin!
            uint8_t buf[2];
            if (uart_read_bytes(UART_NUM_0, buf, 2, 0) == 2) {
                target_read = buf[1] << 8 | buf[0];
                PT_DEVICE->set_rx_led(true);
                is_reading = true;
                read_pos = 0;
            }
        } else if (avaliable > 0 && is_reading) {
            int max_to_read = std::min((size_t)(target_read - read_pos), avaliable);
            int actual_read = uart_read_bytes(UART_NUM_0, &tmp_queue[read_pos], max_to_read, 0);
            read_pos += actual_read;

            if (read_pos == target_read) {
                // Payload complete!
                is_reading = false;
                rx_tmp.arg_size = target_read - 2;
                rx_tmp.msg_id = tmp_queue[0];
                rx_tmp.msg_type = tmp_queue[1];
                memcpy(&rx_tmp.args[0], &tmp_queue[2], target_read-2);
                receive_change = true;
                PT_DEVICE->set_rx_led(false);
            }
        }
        vTaskDelay(1);
    }
}


namespace PCCOMM {
    int last_id = 0;
    bool read_message(COMM_MSG *msg) {
        //if (receive_change) {
        //    memcpy(msg, &rx_tmp, sizeof(COMM_MSG));
        //    receive_change = false;
        //    if (msg->msg_id != 0x00) {
        //       last_id = msg->msg_id;
        //    }
        //    return true;
        //} else {
        //    return false;
        //}
    
        size_t avaliable;
        uart_get_buffered_data_len(UART_NUM_0, &avaliable);
        if (avaliable > 2 && !is_reading) {
            // Payload begin!
            uint8_t buf[2];
            if (uart_read_bytes(UART_NUM_0, buf, 2, 0) == 2) {
                target_read = buf[1] << 8 | buf[0];
                PT_DEVICE->set_rx_led(true);
                is_reading = true;
                read_pos = 0;
            }
        } else if (avaliable > 0 && is_reading) {
            int max_to_read = std::min((size_t)(target_read - read_pos), avaliable);
            int actual_read = uart_read_bytes(UART_NUM_0, &tmp_queue[read_pos], max_to_read, 0);
            read_pos += actual_read;

            if (read_pos == target_read) {
                // Payload complete!
                is_reading = false;
                msg->arg_size = target_read - 2;
                msg->msg_id = tmp_queue[0];
                msg->msg_type = tmp_queue[1];
                if (msg->msg_id != 0) {
                    last_id = msg->msg_id;
                }
                memcpy(&msg->args[0], &tmp_queue[2], target_read-2);
                PT_DEVICE->set_rx_led(false);
                return true;
            }
        }
        return false;
    }


    void init() {
        uart_config_t cfg;
        cfg.baud_rate = 2000000;
        cfg.data_bits = UART_DATA_8_BITS;
        cfg.parity = UART_PARITY_DISABLE;
        cfg.stop_bits = UART_STOP_BITS_1;
        cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

        uart_param_config(UART_NUM_0, &cfg);
        uart_driver_install(UART_NUM_0, 8192, 8192, 0, NULL, 0);


        //if (xTaskCreate(loop_rx, "serial_rx_task", 8192, nullptr, configMAX_PRIORITIES-2, NULL) != pdPASS) {
        //    PT_DEVICE->set_rgb_led(153, 50, 204);
        //}
    }

    void send_message(COMM_MSG *msg) {
        PT_DEVICE->set_tx_led(true);
        char* tmp = new char[msg->arg_size + 4];
        tmp[0] = (msg->arg_size + 2);
        tmp[1] = (msg->arg_size + 2) >> 8;
        tmp[2] = msg->msg_id;
        tmp[3] = msg->msg_type;
        memcpy(&tmp[4], &msg->args[0], msg->arg_size);
        uart_write_bytes(UART_NUM_0, (const char*)tmp, msg->arg_size+4); // buffer
        delete[] tmp;
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
        
    }
}

#endif