#include "comm_channels.h"


// Debug function
bool debug_send_frame(CAN_FRAME &f) {
    #ifdef FW_TEST
    char buf[80] = {0x00};
    char *pos = buf;
    pos += sprintf(pos, "Send frame -> %04X (LEN: %d) [", f.id, f.length);
    for (int i = 0; i < f.length; i++) {
        pos+=sprintf(pos, "%02X ", f.data.bytes[i]);
    }
    sprintf(pos-1,"]");
    PCCOMM::log_message(buf);
    #endif
    return CustomCan::sendFrame(&f);
}

bool debug_send_frame_force(CAN_FRAME &f) {
    char buf[80] = {0x00};
    char *pos = buf;
    pos += sprintf(pos, "Send frame -> %04X (LEN: %d) [", f.id, f.length);
    for (int i = 0; i < f.length; i++) {
        pos+=sprintf(pos, "%02X ", f.data.bytes[i]);
    }
    sprintf(pos-1,"]");
    PCCOMM::log_message(buf);
    return CustomCan::sendFrame(&f);
}

void debug_read_frame(CAN_FRAME &f) {
    #ifdef FW_TEST
    char buf[80] = {0x00};
    char *pos = buf;
    pos += sprintf(pos, "Read frame -> %04X (LEN: %d) [", f.id, f.length);
    for (int i = 0; i < f.length; i++) {
        pos+=sprintf(pos, "%02X ", f.data.bytes[i]);
    }
    sprintf(pos-1,"]");
    PCCOMM::log_message(buf);
    #endif
}