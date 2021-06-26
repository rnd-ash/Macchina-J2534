#include "comm_channels.h"
#include "pt_device.h"

bool CanChannel::setup(int id, int protocol, int baud, int flags) {
    // Here we go, setup a CAN channel!
    if (!CustomCan::enableCanBus(baud)) {
         PCCOMM::respond_err(MSG_OPEN_CHANNEL, ERR_FAILED, "CAN Controller setup failed!");
         return false;
    }
    if (flags & CAN_29BIT_ID) { // extended addressing, 
        PCCOMM::log_message("CAN Extended enabled");
        this->isExtended = true;
    } else {
        this->isExtended = false;
    }
    // Can is OK, now blank set all mailboxes to a block state by default
    PT_DEVICE->set_can_led(true);
    this->channel_id = id;
    this->f.length = 0;
    this->loopback = false; // Loopback is disabled by default!
    return true;
}

void CanChannel::addFilter(int type, int filter_id, char* mask, char* pattern, char* flowcontrol, int mask_len, int pattern_len, int flowcontrol_len) {
     if (type == FLOW_CONTROL_FILTER) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_FAILED, "CAN Channel cannot use flow control filter");
        return;
    }
    if (mask_len > 4) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_FAILED, "Mask length too big");
        return;
    }
    if (pattern_len > 4) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_FAILED, "Pattern length too big");
        return;
    }
     if (filter_id >= MAILBOX_COUNT) { // Out of mailboxes!
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_EXCEEDED_LIMIT, nullptr);
        return;
    }
    if (used_mailboxes[filter_id] == true) {
        PCCOMM::respond_err(MSG_SET_CHAN_FILT, ERR_FAILED, "Filter ID in use");
        return;
    }

    uint32_t mask_id = 0x0000;
    uint32_t ptn_id = 0x0000;

    for (int i = 0; i < mask_len; i++) {
        mask_id <<= 8;
        mask_id |= mask[i];
    }

    for (int i = 0; i < pattern_len; i++) {
        ptn_id <<= 8;
        ptn_id |= pattern[i];
    }

    if (type == BLOCK_FILTER) { // Block filter. Set the CAN Filter ID to be open, and then we will block it in software
        CustomCan::enableCanFilter(filter_id, 0x0000, 0x0000, isExtended); // Open the mailbox filter to everything
        blocking_filters[filter_id] = true; // Mark this as yes for the update function
    } else { // Pass filter, use hardware filter
        CustomCan::enableCanFilter(filter_id, ptn_id, mask_id, isExtended); // Open the mailbox filter to everything
        blocking_filters[filter_id] = false;

    }
    patterns[filter_id] = ptn_id;
    masks[filter_id] = mask_id;
    used_mailboxes[filter_id] = true;
    PCCOMM::respond_ok(MSG_SET_CHAN_FILT, nullptr, 0);
}

void CanChannel::update() {
    for (int i = 0; i < MAILBOX_COUNT; i++) { // Check all our filters in use
        if (used_mailboxes[i] == true) { // We should this filter
            if (CustomCan::receiveFrame(i, &f)) {
                bool send_frame = true;
                if (blocking_filters[i] == true) { // Check block filter
                    send_frame = masks[i] & f.id != patterns[i]; // Block filter check
                }
                if (send_frame) { // Frame should be sent to the PC
                    char buf[f.length + 4];
                    // TODO - Rx Flags for CAN - Although i don't think they are needed, so leave them 0x0000
                    uint32_t rx_status = 0x0000;
                    buf[0] = f.id >> 24;
                    buf[1] = f.id >> 16;
                    buf[2] = f.id >> 8;
                    buf[3] = f.id >> 0;
                    memcpy(&buf[4], &f.data.bytes[0], f.length);  // Copy CAN Data
                    PCCOMM::send_rx_data(this->channel_id, rx_status, buf, f.length+4); // Tx to PC
                }
            }
        }
    }
}

void CanChannel::removeFilter(int id) {
    if (this->used_mailboxes[id] == true) {
        this->used_mailboxes[id] = false;
        this->masks[id] = 0;
        this->patterns[id] = 0;
        this->blocking_filters[id] = false;
        CustomCan::disableCanFilter(id);
        PCCOMM::respond_ok(MSG_REM_CHAN_FILT, nullptr, 0);
    } else {
        PCCOMM::respond_err(MSG_REM_CHAN_FILT, ERR_INVALID_FILTER_ID, nullptr);
    }
}

void CanChannel::destroy() {
    CustomCan::disableCanBus();
    PT_DEVICE->set_can_led(false);
}

/**
 * Macchina will NOT respond to this request, just send and leave it
 */
void CanChannel::sendMsg(uint32_t tx_flags, char* data, int data_size, bool respond) {
    // First 4 bytes are CAN ID, followed by the CAN Data
    CAN_FRAME f;
    f.length = data_size - 4;
    f.id = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3] << 0;
    memcpy(&f.data.bytes[0], &data[4], data_size-4);
    CustomCan::sendFrame(&f);
    if (respond) {
        PCCOMM::respond_ok(MSG_TX_CHAN_DATA, nullptr, 0);
    }
    if (this->loopback) {
        PCCOMM::send_rx_data(this->channel_id, TX_MSG_TYPE, data, data_size);
    }
}


void CanChannel::ioctl_get(uint32_t id) {
    PCCOMM::respond_err(MSG_IOCTL_GET, ERR_FAILED, "CAN IOCTL get unimplemented");
}

void CanChannel::ioctl_set(uint32_t id, uint32_t value) {
    if (id == LOOPBACK) {
        if (value == 0) {
            this->loopback = false;
        } else {
            this->loopback = true;
        }
        PCCOMM::respond_ok(MSG_IOCTL_SET, nullptr, 0);
    } else {
        PCCOMM::respond_err(MSG_IOCTL_SET, ERR_FAILED, "CAN IOCTL set unimplemented");
    }
}