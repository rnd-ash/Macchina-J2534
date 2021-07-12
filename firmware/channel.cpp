#include "channel.h"

Channel* canChannel = nullptr; // Channel for physical canbus link
Channel* klineChannel = nullptr; // Channel for physical kline line

int little_endian_decode(uint8_t* src) {
    return src[3] << 24 |
        src[2] << 16 |
        src[1] << 8 |
        src[0];
}


void setup_channel(COMM_MSG* msg) {
    if (msg->msg_type != MSG_OPEN_CHANNEL) {
        PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_FAILED, "This is NOT a open channel msg!");
    }
    if (msg->arg_size != 16) {
        char buf[65];
        sprintf(buf, "Payload size for OpenChannel is incorrect. Want 16, got %d", msg->arg_size);
        PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_FAILED, buf);
    }
    unsigned int id = little_endian_decode(&msg->args[0]);
    unsigned int protocol = little_endian_decode(&msg->args[4]);
    unsigned int baud = little_endian_decode(&msg->args[8]);
    unsigned int flags = little_endian_decode(&msg->args[12]);
    switch (id)
    {
        case CAN_CHANNEL_ID:
            if (canChannel != nullptr) {
                PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_CHANNEL_IN_USE, nullptr);
            } else {
                create_can_channel(id, protocol, baud, flags);
            }
            break;
#ifdef CFG_MACCHINA_M2
        case KLINE_CHANNEL_ID:
            if (klineChannel != nullptr) {
                PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_CHANNEL_IN_USE, nullptr);
            } else {
                create_lin_channel(id, protocol, baud, flags);
            }
            break;
#endif
        default:
            PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_FAILED, "Protocol unsupported");
            break;
    }
}

void create_can_channel(int id, int protocol, int baud, int flags) {
    Channel *c = nullptr;
    if (protocol == ISO15765) { // ISO-TP
        c = new ISO15765Channel();
    } else { // Standard CAN
        c = new CanChannel();
    }
    if (!c->setup(id, protocol, baud, flags)) { // This function will return log the error to driver if any error
        delete c;
        return;
    }
    canChannel = c; // Creation ok!
    PCCOMM::respond_ok(MSG_OPEN_CHANNEL, nullptr, 0); // Tell driver CAN based channel is ready!
}

#if defined(CFG_MACCHINA_M2)
void create_lin_channel(int id, int protocol, int baud, int flags) {
    Channel *c = nullptr;
    if (protocol == ISO9141) {
        c = new Iso9141Channel();
    } else {
        PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_NOT_SUPPORTED, nullptr);
        return;
    }
    if (!c->setup(id, protocol, baud, flags)) { // This function will return log the error to driver if any error
        delete c;
        return;
    }
    klineChannel = c;
    PCCOMM::respond_ok(MSG_OPEN_CHANNEL, nullptr, 0);
}
#endif

void remove_channel(COMM_MSG *msg) {
    if (msg->msg_type != MSG_CLOSE_CHANNEL) {
        PCCOMM::respond_err(MSG_CLOSE_CHANNEL, ERR_FAILED, "This is NOT a close channel msg!");
    }
    if (msg->arg_size != 4) {
        char buf[65];
        sprintf(buf, "Payload size for OpenChannel is incorrect. Want 4, got %d", msg->arg_size);
        PCCOMM::respond_err(MSG_CLOSE_CHANNEL, ERR_FAILED, buf);
    }
    unsigned int id;
    memcpy(&id, &msg->args[0], 4);
    switch(id) {
        case CAN_CHANNEL_ID:
            delete_channel(canChannel);
            PCCOMM::respond_ok(MSG_CLOSE_CHANNEL, nullptr, 0);
            break;
        case KLINE_CHANNEL_ID:
            delete_channel(klineChannel);
            PCCOMM::respond_ok(MSG_CLOSE_CHANNEL, nullptr, 0);
            break;
        default:
            PCCOMM::respond_err(MSG_CLOSE_CHANNEL, ERR_FAILED, "Protocol unsupported");
            break;
    }
}

void delete_channel(Channel*& ptr) {
    if (ptr != nullptr) {
        ptr->destroy();
        delete ptr;
        ptr = nullptr;
        PCCOMM::respond_ok(MSG_CLOSE_CHANNEL, nullptr, 0);
    } else {
        PCCOMM::respond_err(MSG_CLOSE_CHANNEL, ERR_INVALID_CHANNEL_ID, nullptr);
    }
}

void channel_loop() {
    if (canChannel != nullptr) {
        canChannel->update();
    }
    if (klineChannel != nullptr) {
        klineChannel->update();
    }
}

void reset_all_channels() {
     if (canChannel != nullptr) {
        canChannel->destroy();
        delete canChannel;
        canChannel = nullptr;
    }
    if (klineChannel != nullptr) {
        klineChannel->destroy();
        delete klineChannel;
        klineChannel = nullptr;
    }
}

void del_channel_filter(COMM_MSG* msg) {
    if (msg->arg_size != 8) {
        PCCOMM::respond_err(MSG_REM_CHAN_FILT, ERR_FAILED, "Message size not valid");
        return;
    }
    unsigned int channel_id = little_endian_decode(&msg->args[0]);
    unsigned int filter_id = little_endian_decode(&msg->args[4]);

    if (channel_id == CAN_CHANNEL_ID) {
        if (canChannel != nullptr) {
            canChannel->removeFilter(filter_id);
            return;
        } else {
            PCCOMM::respond_err(MSG_REM_CHAN_FILT, ERR_INVALID_CHANNEL_ID, "Can based channel null");
            return;
        }
    } else if (channel_id == KLINE_CHANNEL_ID) {
        if (klineChannel != nullptr) {
            klineChannel->removeFilter(filter_id);
            return;
        } else {
            PCCOMM::respond_err(MSG_REM_CHAN_FILT, ERR_INVALID_CHANNEL_ID, "Kline based channel null");
            return;
        }
    }


}

void add_channel_filter(COMM_MSG* msg) {
    unsigned int channel_id = little_endian_decode(&msg->args[0]);
    unsigned int filter_id = little_endian_decode(&msg->args[4]);
    unsigned int filter_type = little_endian_decode(&msg->args[8]);
    unsigned int mask_size = little_endian_decode(&msg->args[12]);
    unsigned int pattern_size = little_endian_decode(&msg->args[16]);
    unsigned int flowcontrol_size = little_endian_decode(&msg->args[20]);
    if (filter_type == FLOW_CONTROL_FILTER && flowcontrol_size == 0) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_NULL_PARAMETER, "WTF. ISO15765 FC filter is null? Driver should have checked this!");
        return;
    }
    // Check if the channel is valid?
    if (channel_id != CAN_CHANNEL_ID && channel_id != KLINE_CHANNEL_ID) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_INVALID_CHANNEL_ID, "Channel ID does not exist");
        return;
    }

    // Channel is valid - Create our arrays for filter messages

    // Mask
    char* mask = new char[mask_size];
    memcpy(&mask[0], &msg->args[24], mask_size);

    // Pattern
    char* pattern = new char[pattern_size];
    memcpy(&pattern[0], &msg->args[24+mask_size], pattern_size);

    // This is the only optional filter
    char* flowcontrol = nullptr;
    if (flowcontrol_size > 0) {
        flowcontrol = new char[flowcontrol_size];
        memcpy(&flowcontrol[0], &msg->args[24+mask_size+pattern_size], flowcontrol_size);
    }

    if (channel_id == CAN_CHANNEL_ID) {
        if (canChannel != nullptr) {
            canChannel->addFilter(filter_type, filter_id, mask, pattern, flowcontrol, mask_size, pattern_size, flowcontrol_size);
        } else {
            PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_INVALID_CHANNEL_ID, nullptr);
        }
    } else if (channel_id == KLINE_CHANNEL_ID) {
        if (klineChannel != nullptr) {
             klineChannel->addFilter(filter_type, filter_id, mask, pattern, flowcontrol, mask_size, pattern_size, flowcontrol_size);
        } else {
             PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_INVALID_CHANNEL_ID, nullptr);
        }
    }
    // Done with these arrays, hardware has applied them, destroy
    delete[] mask;
    delete[] pattern;
    if (flowcontrol != nullptr) {
        delete[] flowcontrol;
    }
}

void send_data(COMM_MSG *msg) {
    bool require_response = msg->msg_id != 0x00;
    uint32_t channel_id;
    uint32_t tx_flags;

    uint32_t data_size = msg->arg_size - 8;
    char* buf = new char[data_size];

    memcpy(&channel_id, &msg->args[0], 4);
    memcpy(&tx_flags, &msg->args[4], 4);
    memcpy(&buf[0], &msg->args[8], data_size);
    if (channel_id == CAN_CHANNEL_ID) {
        if (canChannel != nullptr) {
            canChannel->sendMsg(tx_flags, buf, data_size, require_response);
        } else {
            if (require_response) {
                PCCOMM::respond_err(MSG_TX_CHAN_DATA, ERR_INVALID_CHANNEL_ID, nullptr);
            } else {
                PCCOMM::log_message("Cannot send, Channel null!");
            }
        }
    } else if (channel_id == KLINE_CHANNEL_ID) {
        if (klineChannel != nullptr) {
            klineChannel->sendMsg(tx_flags, buf, data_size, require_response);
        } else {
            if (require_response) {
                PCCOMM::respond_err(MSG_TX_CHAN_DATA, ERR_INVALID_CHANNEL_ID, nullptr);
            } else {
                PCCOMM::log_message("Cannot send, Channel null!");
            }
        }
    } else {
        if (require_response) {
             PCCOMM::respond_err(MSG_TX_CHAN_DATA, ERR_FAILED, "Tx data not implemented for this protocol");
        } else {
            PCCOMM::log_message("Cannot send, not implemented!");
        }
    }
    delete[] buf;
}


void ioctl_get(COMM_MSG *msg) {
    uint8_t channel_id;
    uint32_t ioctl_id;
    if (msg->arg_size != 5) {
        PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "IOCTL get request invalid length");
        return;
    }
    channel_id = msg->args[0];
    memcpy(&ioctl_id, &msg->args[1], 4);
    switch (channel_id)
    {
    case CAN_CHANNEL_ID:
        if (canChannel != nullptr) {
            canChannel->ioctl_get(ioctl_id);
        } else {
            PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "Can channel is null!");
        }
        break;
    case KLINE_CHANNEL_ID:
        if (klineChannel != nullptr) {
            klineChannel->ioctl_get(ioctl_id);
        } else {
            PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "Can channel is null!");
        }
        break;
    
    default:
        PCCOMM::respond_err(MSG_IOCTL_GET, ERR_INVALID_CHANNEL_ID, nullptr);
        break;
    }
}

void ioctl_set(COMM_MSG *msg) {
    uint8_t channel_id;
    uint32_t ioctl_id;
    uint32_t value;
    if (msg->arg_size != 9) {
        PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "IOCTL set request invalid length");
        return;
    }
    channel_id = msg->args[0];
    memcpy(&ioctl_id, &msg->args[1], 4);
    memcpy(&value, &msg->args[5], 4);
    switch (channel_id)
    {
    case CAN_CHANNEL_ID:
        if (canChannel != nullptr) {
            canChannel->ioctl_set(ioctl_id, value);
        } else {
            PCCOMM::respond_err(MSG_IOCTL_SET, ERR_FAILED, "Can channel is null!");
        }
        break;
    case KLINE_CHANNEL_ID:
        if (klineChannel != nullptr) {
            klineChannel->ioctl_set(ioctl_id, value);
        } else {
            PCCOMM::respond_err(MSG_IOCTL_SET, ERR_FAILED, "Can channel is null!");
        }
        break;
    default:
        PCCOMM::respond_err(MSG_IOCTL_SET, ERR_INVALID_CHANNEL_ID, nullptr);
        break;
    }
}

// Fast init or Five baud init
void init_lin_channel(COMM_MSG *msg) {
    unsigned long channel_id = little_endian_decode(&msg->args[0]); // 0-4
    uint8_t init_method = msg->args[4]; // FIVE_BAUD = 0, FAST = 1
    uint8_t* args = &msg->args[5];
    uint8_t args_size = msg->arg_size - 5;

    if (channel_id != KLINE_CHANNEL_ID) { // ID mismatch for K-Line
        PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_INVALID_CHANNEL_ID, nullptr);
    }
    // Check if KLINE is null
    if (klineChannel == nullptr) {
        PCCOMM::respond_err(MSG_INIT_LIN_CHANNEL, ERR_INVALID_CHANNEL_ID, nullptr);
    } else {
        klineChannel->wakeup(init_method, args, args_size);
    }
}
